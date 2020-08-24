#ifndef DUNE_COPASI_ENUM_HH
#define DUNE_COPASI_ENUM_HH

#include <dune/copasi/common/bit_flags.hh>

namespace Dune::Copasi {

namespace ModelSetup {

//! List of stages in a model setup
enum class Stages
{
  GridFunctionSpace = 1 << 1,
  CoefficientVector = 1 << 2,
  InitialCondition = 1 << 3,
  Constraints = 1 << 4,
  LocalOperator = 1 << 5,
  GridOperator = 1 << 6,
  Solver = 1 << 7,
  Writer = 1 << 8
};

//! Policy that setup the grid function spaces including its dependencies in a
//! model
constexpr Stages setup_grid_function_space = Stages::GridFunctionSpace;

//! Policy that setup the coefficient vectors including its dependencies in a
//! model
constexpr Stages setup_coefficient_vector =
  setup_grid_function_space | Stages::CoefficientVector;

//! Policy that setup the initial condition including its dependencies in a
//! model
constexpr Stages setup_initial_condition =
  setup_coefficient_vector | Stages::InitialCondition;

//! Policy that setup the constraints including its dependencies in a model
constexpr Stages setup_constraints =
  setup_coefficient_vector | Stages::Constraints;

//! Policy that setup the local operator including its dependencies in a model
constexpr Stages setup_local_operator = setup_constraints | Stages::Constraints;

//! Policy that setup the grid operator including its dependencies in a model
constexpr Stages setup_grid_operator =
  setup_local_operator | Stages::GridOperator;

//! Policy that setup the solver including its dependencies in a model
constexpr Stages setup_solver = setup_grid_operator | Stages::Solver;

//! Policy that setup the writer including its dependencies in a model
constexpr Stages setup_writer = setup_coefficient_vector | Stages::Writer;

} // namespace ModelSetup

/**
 * @brief      This class describes an adaptivity policy.
 */
enum class AdaptivityPolicy
{
  None
};

/**
 * @brief      This class describes a jacobian method.
 */
enum class JacobianMethod
{
  Analytical,
  Numerical
};

} // namespace Dune::Copasi

#endif // DUNE_COPASI_ENUM_HH
