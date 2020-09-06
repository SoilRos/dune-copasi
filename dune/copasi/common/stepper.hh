#ifndef DUNE_COPASI_COMMON_STEPPERS_HH
#define DUNE_COPASI_COMMON_STEPPERS_HH

// this is because without it name lookup fails to find PDELab::native(...)
#include <dune/pdelab/backend/istl.hh>

#include <dune/pdelab/instationary/onestep.hh>
#include <dune/pdelab/newton/newton.hh>

#include <dune/logging.hh>

#include <dune/common/float_cmp.hh>
#include <dune/common/parametertree.hh>

#include <any>
#include <memory>
#include <tuple>

namespace Dune::Copasi {

/**
 * @brief Base stepper for static polymorphic steppers
 *
 * @details  The setpper implementation must contain the method `do_step` with
 * input and output states. Then, this class will be able to instantiate
 * `do_step` without in/out states and the methods to evolve a system until a
 * final time.
 *
 * @tparam Impl    Stepper implementation
 */
template<class Impl>
class BaseStepper
{
public:
  /**
   * @brief Perform one step in the system
   *
   * @details The input state is advanced a delta time `dt` and placed in `out`
   * state. Signature of `do_step` method that `Impl` must implement
   *
   * @tparam System   System that contain suitable operators to advance in time
   * @tparam State    State to perform time-advance with
   * @tparam Time     Valid time type to operate with
   * @param system    System that contain suitable operators to advance in time
   * @param in        Input state to advance time from
   * @param out       Output state where result will be placed. If step is not
   * possible, out is guaranteed to be false when casted to bool
   * @param dt        Delta time to perform step. If the step is not possible,
   * stepper may modify it to a suitable value
   */
  template<class System, class State, class Time>
  void do_step(System& system, const State& in, State& out, Time& dt) const
  {
    asImpl().do_step(system, in, out, dt);
  }

  /**
   * @brief Perform one step in the system
   *
   * @details The system state is advanced a delta time `dt`
   *
   * @tparam System   System that contain suitable operators to advance in time
   * @tparam Time     Valid time type to operate with
   * @param system    System that contain suitable operators to advance in time.
   * If the step is not possible, the system stays with its initial state
   * @param dt        Delta time to perform step. If the step is not possible,
   * stepper may modify it to a suitable value
   */
  template<class System, class Time>
  void do_step(System& system, Time& dt) const
  {
    auto in = system.state();
    auto out = in;
    do_step(system, in, out, dt);
    if (out)
      system.set_state(out);
  }

  /**
   * @brief Evolve the system until `end_time`
   *
   * @details The input state is advanced by time steps until `end_time` is
   * reached and placed in `out` state.
   *
   * @tparam System   System that contain suitable operators to advance in time
   * @tparam State    State to perform time-advance with
   * @tparam Time     Valid time type to operate with
   * @param system    System that contain suitable operators to advance in time
   * @param in        Input state to advance time from
   * @param out       Output state where result will be placed. If step is not
   * possible, out is guaranteed to be false when casted to bool
   * @param dt        Delta time to perform first step. If the step is not
   * possible, stepper may modify it to a suitable value
   * @param end_time  Final time that `out` state must reach
   * @param callable  A function called with an state at the end of each
   * succesful step
   */
  template<class System, class State, class Time, class Callable>
  void evolve(
    System& system,
    const State& in,
    State& out,
    Time& dt,
    const Time& end_time,
    Callable&& callable = [](const auto& state) {}) const
  {
    assert(FloatCmp::lt<double>(in.time, end_time));
    const auto& logger = asImpl().logger();
    logger.notice("Evolving system: {:.2f}s -> {:.2f}s"_fmt, in.time, end_time);
    out = in;
    while (FloatCmp::lt<double>(out.time, end_time)) {
      if (FloatCmp::gt<double>(dt, end_time - out.time)) {
        logger.detail("Reduce step to match end time: {:.2f}s -> {:.2f}"_fmt,
                      dt,
                      end_time - out.time);
        dt = end_time - out.time;
      }
      do_step(system, out, out, dt);
      if (not out) {
        logger.warn("Evolving system could not reach final time"_fmt);
        break;
      }
      callable(out);
    }
  }

  /**
   * @brief Evolve the system until `end_time`
   *
   * @details The system state is advanced by time steps until `end_time`
   *
   * @tparam System   System that contain suitable operators to advance in time
   * @tparam Time     Valid time type to operate with
   * @param system    System that contain suitable operators to advance in time.
   * If the step is not possible, the system stays with its initial state
   * @param dt        Delta time to perform first step. If the step is not
   * possible, stepper may modify it to a suitable value
   * @param end_time  Final time that `out` state must reach
   * @param callable  A function called with an state at the end of each
   * succesful step
   */
  template<class System, class Time, class Callable>
  void evolve(
    System& system,
    Time& dt,
    const Time& end_time,
    Callable&& callable = [](const auto& state) {}) const
  {
    auto in = system.state();
    auto out = in;
    evolve(system, in, out, dt, end_time, callable);
    if (out)
      system.set_state(out);
  }

private:
  //! Cast to implementation (Barton–Nackman trick)
  const Impl& asImpl() const { return static_cast<const Impl&>(*this); }
};

/**
 * @brief Runge-Kutta stepper for PDEs
 *
 * @details This class is able to advance in time PDE models from PDELab
 * implemented in dune-copasi using [Runge-Kutta
 * methods](https://en.wikipedia.org/wiki/List_of_Runge%E2%80%93Kutta_methods).
 * Fully implicit Runge-Kutta are not available.
 *
 * @warning Due the PDELab objects lifetime, advancing in time may modify system
 * local and grid operators. Thus, when applying steps to different states, be
 * sure that system operators are semantically const correct
 *
 * @tparam T    Time type to step systems with
 */
template<class T = double>
class RKStepper : public BaseStepper<RKStepper<T>>
{
public:
  using Time = T;

  /**
   * @brief Construct a new object
   *
   * @param rk_method   PDELab time stepping Runge-Kutta paramameters
   */
  RKStepper(
    std::unique_ptr<PDELab::TimeSteppingParameterInterface<Time>> rk_method)
    : _rk_method{ std::move(rk_method) }
    , _logger{ Logging::Logging::componentLogger({}, "stepper") }
  {
    _logger.detail("Setting up time stepper"_fmt);
    _logger.trace("Stepper methd: {}"_fmt, _rk_method->name());
  }

  /**
   * @brief Construct a new object
   *
   * @param rk_method   Keyword with the RK method type
   */
  RKStepper( std::string rk_method)
    : RKStepper(make_rk_method(rk_method))
  {}

  /**
   * @brief Make RK parameters
   *
   * @param type  a Keyword with the RK method type
   * @return auto  a pointer to PDELab time stepping Runge-Kutta paramameters
   */
  static std::unique_ptr<PDELab::TimeSteppingParameterInterface<Time>>
  make_rk_method(const std::string type)
  {
    if (type == "explicit_euler")
      return std::make_unique<PDELab::ExplicitEulerParameter<Time>>();
    if (type == "implicit_euler")
      return std::make_unique<PDELab::ImplicitEulerParameter<Time>>();
    if (type == "heun")
      return std::make_unique<PDELab::HeunParameter<Time>>();
    if (type == "shu_3")
      return std::make_unique<PDELab::Shu3Parameter<Time>>();
    if (type == "runge_kutta_4")
      return std::make_unique<PDELab::RK4Parameter<Time>>();
    if (type == "alexander_2")
      return std::make_unique<PDELab::Alexander2Parameter<Time>>();
    if (type == "fractional_step_theta")
      return std::make_unique<PDELab::FractionalStepParameter<Time>>();
    if (type == "alexander_3")
      return std::make_unique<PDELab::Alexander3Parameter<Time>>();

    DUNE_THROW(NotImplemented, "Not known '" << type << "' Runge Kutta method");
  }

  //! @copydoc BaseStepper::do_step()
  template<class System>
  void do_step(System& system, Time& dt) const
  {
    BaseStepper<RKStepper<T>>::do_step(system, dt);
  }

  //! @copydoc BaseStepper::do_step()
  template<class System, class State>
  void do_step(const System& system,
               const State& in,
               State& out,
               Time& dt) const
  {
    assert(out.grid_function_space and out.grid);

    using Coefficients = typename State::Coefficients;
    const bool cout_redirected = Dune::Logging::Logging::isCoutRedirected();
    if (not cout_redirected)
      Dune::Logging::Logging::redirectCout(_logger.name(),
                                           Dune::Logging::LogLevel::detail);

    _logger.detail("Trying step: {:.2f}s + {:.2f}s -> {:.2f}s"_fmt,
                   in.time,
                   dt,
                   in.time + dt);

    auto& solver = get_solver(system);

    const auto& x_in = in.coefficients;
    auto& x_out = out.coefficients;
    if (not x_out or x_out == x_in)
      x_out = std::make_shared<Coefficients>(*x_in);

    // try & catch is the only way we have to check if the solver was successful
    try {
      solver.apply(in.time, dt, *x_out, *x_out);
      solver.result(); // triggers an exception if solver failed

      _logger.notice("Time Step: {:.2f}s + {:.2f}s -> {:.2f}s"_fmt,
                     in.time,
                     dt,
                     in.time + dt);
      out.time = in.time + dt;

    } catch (const Dune::PDELab::NewtonError& e) {
      out.coefficients.reset(); // set output to an invalid state
      _logger.warn("Step failed"_fmt);
      _logger.trace("{}"_fmt, e.what());
    }

    if (not cout_redirected)
      Dune::Logging::Logging::restoreCout();
  }

  //! Return stepper logger
  const Logging::Logger& logger() const { return _logger; }

private:
  //! Setup and cache solver for a system
  template<class System>
  auto& get_solver(const System& system) const
  {
    using Coefficients = typename System::State::Coefficients;
    using GridOperator = typename System::GridOperator;
    using JacobianOperator = typename System::JacobianOperator;
    using NonLinearOperator =
      PDELab::Newton<GridOperator, JacobianOperator, Coefficients>;
    using OneStepOperator = PDELab::
      OneStepMethod<Time, GridOperator, NonLinearOperator, Coefficients>;

    using InternalState = std::tuple<System const*,
                                     GridOperator const*,
                                     JacobianOperator const*,
                                     std::shared_ptr<NonLinearOperator>,
                                     std::shared_ptr<OneStepOperator>>;

    GridOperator& grid_operator = *system.get_grid_operator();
    JacobianOperator& jacobian_operator = *system.get_jacobian_operator();

    // If internal data correspond to input, return cached one-step-operator.
    if (_internal_state.has_value() and
        _internal_state.type() == typeid(InternalState)) {
      auto& internal_state = *std::any_cast<InternalState>(&_internal_state);
      if (std::get<0>(internal_state) == &system and
          std::get<1>(internal_state) == &grid_operator and
          std::get<2>(internal_state) == &jacobian_operator)
        return *std::get<4>(internal_state);
    }

    _logger.trace("Get non-linear operator"_fmt);
    auto non_linear_operator =
      std::make_shared<NonLinearOperator>(grid_operator, jacobian_operator);

    _logger.trace("Get one step operator"_fmt);
    auto one_step_operator = std::make_shared<OneStepOperator>(
      *_rk_method, grid_operator, *non_linear_operator);
    one_step_operator->setVerbosityLevel(2);

    _internal_state =
      std::make_any<InternalState>(&system,
                                   &grid_operator,
                                   &jacobian_operator,
                                   std::move(non_linear_operator),
                                   std::move(one_step_operator));
    return *std::get<4>(*std::any_cast<InternalState>(&_internal_state));
  }

private:
  std::unique_ptr<const PDELab::TimeSteppingParameterInterface<Time>>
    _rk_method;
  const Logging::Logger _logger;
  mutable std::any _internal_state;
};

/**
 * @brief Adapt an stepper into a simple adaptive time stepper
 *
 * @details   When an step is not successful, the delta time will be decreased
 * until step is successful or minimum time step is reached. When step is
 * successful, delta time is increased up to a maximum value
 *
 * @tparam SimpleStepper  Stepper that does not addapt its delta time on failure
 */
template<class SimpleStepper>
class SimpleAdaptiveStepper
  : public BaseStepper<SimpleAdaptiveStepper<SimpleStepper>>
{
public:
  using Time = typename SimpleStepper::Time;

  /**
   * @brief Construct a new Simple Adaptive Stepper object
   *
   * @param stepper Stepper to repeat steps with
   * @param min_step Minimum step size
   * @param max_step Maximum step size
   * @param decrease_factor Factor to decrease time step on failure
   * @param increase_factor Factor to increase time step on success
   */
  SimpleAdaptiveStepper(SimpleStepper&& stepper,
                        Time min_step,
                        Time max_step,
                        double decrease_factor = 0.5,
                        double increase_factor = 1.5)
    : _stepper{ std::move(stepper) }
    , _min_step{ min_step }
    , _max_step{ max_step }
    , _decrease_factor{ decrease_factor }
    , _increase_factor{ increase_factor }
  {
    assert(FloatCmp::lt(decrease_factor, 1.));
    assert(FloatCmp::gt(increase_factor, 1.));
  }

  //! @copydoc BaseStepper::do_step()
  template<class System>
  void do_step(System& system, Time& dt) const
  {
    BaseStepper<SimpleAdaptiveStepper<SimpleStepper>>::do_step(system, dt);
  }

  //! @copydoc BaseStepper::do_step()
  template<class System, class State>
  void do_step(const System& system,
               const State& in,
               State& out,
               Time& dt) const
  {
    if (Dune::FloatCmp::lt<Time>(dt, _min_step))
      DUNE_THROW(InvalidStateException,
                 "time-step '" << dt
                               << "' is less than the minimum allowed step '"
                               << _min_step << "'");

    if (Dune::FloatCmp::gt<Time>(dt, _max_step))
      DUNE_THROW(InvalidStateException,
                 "time-step '" << dt
                               << "' is greater than the maximum allowed step '"
                               << _min_step << "'");

    _stepper.do_step(system, in, out, dt);
    while (not out) {
      logger().warn(
        "Reducing step size: {}s -> {}s"_fmt, dt, dt * _decrease_factor);
      dt = dt * _decrease_factor;
      if (FloatCmp::lt<Time>(_min_step, dt))
        DUNE_THROW(MathError,
                   "Time step '" << dt
                                 << "' is less than the minimun allowed step '"
                                 << _min_step << "'");
      _stepper.do_step(system, in, out, dt);
    }

    auto new_step = std::min<Time>(_max_step, dt * _increase_factor);
    logger().detail(
      "Increasing step size: {:.2f}s -> {:.2f}s"_fmt, dt, new_step);
    dt = new_step;
  }

  //! Return stepper logger
  const Logging::Logger& logger() const { return _stepper.logger(); }

private:
  const SimpleStepper _stepper;
  const Time _min_step, _max_step;
  const double _decrease_factor, _increase_factor;
};

template<class Time = double>
auto
make_default_stepper(const ParameterTree& config)
{
  auto log = Logging::Logging::componentLogger({}, "stepper");
  auto rk_type = config.get("rk_method","alexander_2");
  auto min_step = config.template get<double>("min_step");
  auto max_step = config.template get<double>("max_step");
  auto decrease_factor = config.get("decrease_factor", 0.9);
  auto increase_factor = config.get("increase_factor", 1.1);

  log.trace("Increase factor: {}"_fmt,increase_factor);
  log.trace("Decrease factor: {}"_fmt,decrease_factor);
  log.trace("Runge-Kutta method: {}"_fmt,rk_type);

  using RKStepper = Dune::Copasi::RKStepper<double>;
  auto rk_stepper = RKStepper{ rk_type };
  using Stepper = Dune::Copasi::SimpleAdaptiveStepper<RKStepper>;
  return Stepper{
    std::move(rk_stepper), min_step, max_step, decrease_factor, increase_factor
  };
}

} // namespace Dune::Copasi

#endif // DUNE_COPASI_COMMON_STEPPERS_HH
