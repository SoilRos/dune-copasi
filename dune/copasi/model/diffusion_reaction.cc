#ifndef DUNE_COPASI_MODEL_DIFFUSION_REACTION_CC
#define DUNE_COPASI_MODEL_DIFFUSION_REACTION_CC

/**
 * This is the implementation of the ModelDiffusionReaction,
 * particularly, you want to notice that this is not an normal .cc
 * file but a header which has to be included when compiling.
 */

#include <dune/copasi/common/bit_flags.hh>
#include <dune/copasi/common/data_context.hh>
#include <dune/copasi/common/muparser_data_handler.hh>
#include <dune/copasi/common/pdelab_expression_adapter.hh>
#include <dune/copasi/common/tiff_grayscale.hh>
#include <dune/copasi/concepts/pdelab.hh>
#include <dune/copasi/model/diffusion_reaction.hh>

#include <dune/pdelab/function/callableadapter.hh>
#include <dune/pdelab/gridfunctionspace/vtk.hh>

#include <string>

#include <sys/stat.h>

namespace Dune::Copasi {

template<class Traits>
ModelDiffusionReaction<Traits>::ModelDiffusionReaction(
  std::shared_ptr<Grid> grid,
  const Dune::ParameterTree& config,
  GV grid_view,
  BitFlags<ModelSetup::Stages> setup_policy)
  : ModelBase(config)
  , _solver_logger(Logging::Logging::componentLogger(config, "solver"))
  , _components(config.sub("reaction").getValueKeys().size())
  , _config(config)
  , _grid_view(grid_view)
  , _grid(grid)
{
  setup(setup_policy);
  write_states();

  _logger.debug("ModelDiffusionReaction constructed"_fmt);
}

template<class Traits>
ModelDiffusionReaction<Traits>::~ModelDiffusionReaction()
{
  _logger.debug("ModelDiffusionReaction deconstructed"_fmt);
}

template<class Traits>
template<class GFGridView>
auto
ModelDiffusionReaction<Traits>::get_muparser_initial(
  const ParameterTree& model_config,
  const GFGridView& gf_grid_view,
  bool compile)
{
  if (not model_config.hasSub("initial"))
    DUNE_THROW(IOError, "configuration file does not have initial section");

  return get_muparser_expressions(
    model_config.sub("initial"), gf_grid_view, compile);
}

template<class Traits>
template<class GF>
void
ModelDiffusionReaction<Traits>::set_initial(
  const std::vector<std::shared_ptr<GF>>& initial)
{
  using GridFunction = std::decay_t<GF>;
  static_assert(Concept::isPDELabGridFunction<GridFunction>(),
                "GridFunction is not a PDElab grid functions");
  static_assert(
    std::is_same_v<typename GridFunction::Traits::GridViewType, GV>,
    "GridFunction has to have the same grid view as the templated grid view");
  static_assert(
    std::is_same_v<typename GridFunction::Traits::GridViewType,
                   typename Grid::Traits::LeafGridView>,
    "GridFunction has to have the same grid view as the templated grid");
  static_assert((int)GridFunction::Traits::dimDomain == (int)Grid::dimension,
                "GridFunction has to have domain dimension equal to the grid");
  static_assert(GridFunction::Traits::dimRange == 1,
                "GridFunction has to have range dimension equal to 1");

  _logger.debug("set initial state from grid functions"_fmt);

  if (initial.size() != _config.sub("diffusion").getValueKeys().size())
    DUNE_THROW(RangeError, "Wrong number of grid functions");

  for (auto& [op, state] : _states) {
    _logger.trace("interpolation of operator {}"_fmt, op);
    std::size_t comp_size = state.grid_function_space->degree();
    std::vector<std::shared_ptr<GridFunction>> functions(comp_size);

    auto& operator_config = _config.sub("operator");
    auto comp_names = operator_config.getValueKeys();
    std::sort(comp_names.begin(), comp_names.end());

    std::size_t count_i = 0; // index for variables in operator
    std::size_t count_j = 0; // index for all variables
    for (const auto& var : comp_names) {
      assert(count_i < comp_names.size());
      if (op != operator_config.template get<std::size_t>(var))
        continue;

      _logger.trace("creating grid function for variable: {}"_fmt, var);
      functions[count_i] = initial[count_j];
      functions[count_i]->set_time(current_time());
      count_i++;
      count_j++;
    }

    PDELab::DynamicPowerGridFunction<GridFunction> comp_initial(functions);
    Dune::PDELab::interpolate(
      comp_initial, *state.grid_function_space, *state.coefficients);
  }
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::setup_component_grid_function_space(
  std::string name) const
{
  _logger.trace("create a finite element map"_fmt);

  // create entity mapper context
  using Entity = typename Traits::Grid::LeafGridView::template Codim<0>::Entity;
  using Index = std::size_t;
  using EntityMapper = std::function<Index(Entity)>;
  EntityMapper&& em = [](const auto entity) {
    return entity.geometry().type().isCube() ? 0 : 1;
  };

  // get common geometry type on gridview
  if (not has_single_geometry_type(_grid_view))
    DUNE_THROW(InvalidStateException,
               "Grid view has to have only one geometry type");
  GeometryType&& gt = _grid_view.template begin<0>()->geometry().type();

  // create data context with entity mapper, geometry type and grid view
  auto&& ctx = Context::data_context(em, gt, _grid_view);

  // create fem from factory
  std::shared_ptr<FEM> finite_element_map(Factory<FEM>::create(ctx));

  std::size_t order;
  if constexpr (Concept::isMultiDomainGrid<typename Traits::Grid>()) {
    order = finite_element_map
              ->find(_grid->multiDomainEntity(*_grid_view.template begin<0>()))
              .localBasis()
              .order();
  } else
    order = finite_element_map->find(*_grid_view.template begin<0>())
              .localBasis()
              .order();

  _logger.trace("setup grid function space for component {}"_fmt, name);
  const ES entity_set(_grid->leafGridView());
  auto comp_gfs = std::make_shared<LGFS>(entity_set, finite_element_map);
  comp_gfs->name(name);
  comp_gfs->setDataSetType(
    order == 0 ? PDELab::GridFunctionOutputParameters::Output::cellData
               : PDELab::GridFunctionOutputParameters::Output::vertexData);
  return comp_gfs;
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::setup_domain_grid_function_space(
  std::vector<std::string> comp_names) const
{
  _logger.debug("setup domain grid function space"_fmt);

  typename GFS::NodeStorage comp_gfs_vec(comp_names.size());

  for (std::size_t k = 0; k < comp_names.size(); k++) {
    comp_gfs_vec[k] = setup_component_grid_function_space(comp_names[k]);
  }

  _logger.trace("setup power grid function space"_fmt);
  return std::make_shared<GFS>(comp_gfs_vec);
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_grid_function_space()
{
  // get component names
  auto& operator_splitting_config = _config.sub("operator");
  auto comp_names = operator_splitting_config.getValueKeys();
  std::sort(comp_names.begin(), comp_names.end());

  _states.clear();
  // map operator -> variable_name
  _operator_splitting.clear();
  for (auto&& var : comp_names) {
    std::size_t op = operator_splitting_config.template get<std::size_t>(var);
    _operator_splitting.insert(std::make_pair(op, var));
    _states[op] = {}; // initializate map with empty states
    _states[op].grid = _grid;
  }

  for (auto& [op, state] : _states) {
    _logger.trace("setup grid function space for operator {}"_fmt, op);
    auto op_range = _operator_splitting.equal_range(op);
    std::size_t op_size = std::distance(op_range.first, op_range.second);
    assert(op_size > 0);
    std::vector<std::string> op_comp_names(op_size);
    std::transform(op_range.first,
                   op_range.second,
                   op_comp_names.begin(),
                   [](const auto& i) { return i.second; });
    _states[op].grid_function_space =
      setup_domain_grid_function_space(op_comp_names);
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_coefficient_vectors()
{
  for (auto& [op, state] : _states) {
    _logger.debug("setup coefficient vector for operator {}"_fmt, op);
    const auto& gfs = _states[op].grid_function_space;
    auto& x = _states[op].coefficients;
    if (x)
      x = std::make_shared<X>(*gfs, *(x->storage()));
    else
      x = std::make_shared<X>(*gfs);
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_initial_condition()
{
  // if this is a sub model, instantiation of the following instantiation will fail
  if constexpr (not Traits::is_sub_model) {
    // get TIFF data if available
    MuParserDataHandler<TIFFGrayscale<unsigned short>> mu_data_handler;
    if (_config.hasSub("data"))
      mu_data_handler.add_tiff_functions(_config.sub("data"));

    // configure math parsers for initial conditions on each component
    auto initial_muparser = get_muparser_initial(_config, _grid_view, false);

    for (auto&& mu_grid_function : initial_muparser) {
      // make TIFF expression available in the parser
      mu_data_handler.set_functions(mu_grid_function->parser());
      mu_grid_function->set_time(current_time());
      mu_grid_function->compile_parser();
    }
    // evaluate compiled expressions as initial conditions
    set_initial(initial_muparser);
  } else {
    DUNE_THROW(NotImplemented,
               "Initial condition setup is only available for complete models");
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_constraints()
{
  _logger.debug("setup constraints"_fmt);

  auto b0lambda = [&](const auto& i, const auto& x) { return false; };
  auto b0 =
    Dune::PDELab::makeBoundaryConditionFromCallable(_grid_view, b0lambda);

  _logger.trace("assemble constraints"_fmt);
  for (auto& [op, state] : _states) {
    _constraints[op] = std::make_unique<CC>();
    auto& gfs = _states[op].grid_function_space;
    Dune::PDELab::constraints(b0, *gfs, *_constraints[op]);

    _logger.info("constrained dofs in operator {}: {} of {}"_fmt,
                 op,
                 _constraints[op]->size(),
                 gfs->globalSize());
  }
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::setup_local_operator(std::size_t i) const
{
  _logger.trace("setup local operators {}"_fmt, i);

  _logger.trace("create spatial local operator {}"_fmt, i);

  auto local_operator = std::make_shared<LOP>(_grid_view, _config, i);

  _logger.trace("create temporal local operator {}"_fmt, i);
  auto temporal_local_operator = std::make_shared<TLOP>(_grid_view, _config, i);

  return std::make_pair(local_operator, temporal_local_operator);
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_local_operators()
{
  _logger.trace("setup local operators"_fmt);

  _local_operators.clear();
  _temporal_local_operators.clear();

  for (auto& [op, state] : _states) {
    auto operators = setup_local_operator(op);
    _local_operators[op] = operators.first;
    _temporal_local_operators[op] = operators.second;
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_grid_operators()
{
  _logger.debug("setup grid operators"_fmt);
  _spatial_grid_operators.clear();
  _temporal_grid_operators.clear();
  _grid_operators.clear();

  for (auto& [op, state] : _states) {
    auto& gfs = _states[op].grid_function_space;
    auto& lop = _local_operators[op];
    auto& tlop = _temporal_local_operators[op];

    // @todo fix this estimate for something more accurate
    MBE mbe((int)Dune::power((int)3, (int)Grid::dimension));

    _logger.trace("create spatial grid operator {}"_fmt, op);
    _spatial_grid_operators[op] = std::make_shared<GOS>(
      *gfs, *_constraints[op], *gfs, *_constraints[op], *lop, mbe);

    _logger.trace("create temporal grid operator {}"_fmt, op);
    _temporal_grid_operators[op] = std::make_shared<GOT>(
      *gfs, *_constraints[op], *gfs, *_constraints[op], *tlop, mbe);

    _logger.trace("create instationary grid operator {}"_fmt, op);
    _grid_operators[op] = std::make_shared<GOI>(*_spatial_grid_operators[op],
                                                *_temporal_grid_operators[op]);
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_solvers()
{
  _logger.debug("setup solvers"_fmt);
  _linear_solvers.clear();
  _nonlinear_solvers.clear();
  _time_stepping_methods.clear();
  _one_step_methods.clear();

  for (auto& [op, state] : _states) {
    auto& x = state.coefficients;
    _logger.trace("create linear solver"_fmt);
    _linear_solvers[op] = std::make_shared<LS>(*_grid_operators[op]);

    _logger.trace("create nonlinear solver"_fmt);
    _nonlinear_solvers[op] =
      std::make_shared<NLS>(*_grid_operators[op], *x, *_linear_solvers[op]);

    _logger.trace("select and prepare time-stepping scheme"_fmt);
    using AlexMethod = Dune::PDELab::Alexander2Parameter<double>;
    _time_stepping_methods[op] = std::make_shared<AlexMethod>();
    _one_step_methods[op] = std::make_shared<OSM>(*_time_stepping_methods[op],
                                                  *_grid_operators[op],
                                                  *_nonlinear_solvers[op]);
    _one_step_methods[op]->setVerbosityLevel(2);
  }
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup_vtk_writer()
{
  // only configure if config writer section exist
  if(not _config.hasSub("writer"))
  {
    _logger.debug("skipping of setup vtk writer"_fmt);
    return;
  } else {
    _logger.debug("setup vtk writer"_fmt);
  }

  _writer = std::make_shared<W>(_grid_view, Dune::VTK::conforming);
  auto config_writer = _config.sub("writer");
  std::string file_name = config_writer.template get<std::string>("file_name");
  std::string path = config_writer.get("path", file_name);
  struct stat st;

  if (stat(path.c_str(), &st) != 0) {
    int stat = 0;
#if defined(_WIN32)
    stat = mkdir(path.c_str());
#else
    stat = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    if (stat != 0 && stat != -1)
      std::cout << "Error: Cannot create directory " << path << std::endl;
  }

  _sequential_writer = std::make_shared<SW>(_writer, file_name, path, path);
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::suggest_timestep(double dt)
{
  // @todo do time addaptivity
  _config["time_step"] = fmt::format("{:.17e}", dt);
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::setup(BitFlags<ModelSetup::Stages> setup_policy)
{
  if (setup_policy.test(ModelSetup::Stages::GridFunctionSpace))
    setup_grid_function_space();
  if (setup_policy.test(ModelSetup::Stages::CoefficientVector))
    setup_coefficient_vectors();
  if (setup_policy.test(ModelSetup::Stages::InitialCondition))
    setup_initial_condition();
  if (setup_policy.test(ModelSetup::Stages::Constraints))
    setup_constraints();
  if (setup_policy.test(ModelSetup::Stages::LocalOperator))
    setup_local_operators();
  if (setup_policy.test(ModelSetup::Stages::GridOperator))
    setup_grid_operators();
  if (setup_policy.test(ModelSetup::Stages::Solver))
    setup_solvers();
  if (setup_policy.test(ModelSetup::Stages::Writer))
    setup_vtk_writer();
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::step()
{
  double dt = _config.template get<double>("time_step");

  const bool cout_redirected = Dune::Logging::Logging::isCoutRedirected();
  if (not cout_redirected)
    Dune::Logging::Logging::redirectCout(_solver_logger.name(),
                                         Dune::Logging::LogLevel::detail);

  // do time step
  for (auto& [op, state] : _states) {
    _local_operators[op]->update(const_states());

    auto& x = state.coefficients;
    auto x_new = std::make_shared<X>(*x);
    _one_step_methods[op]->apply(current_time(), dt, *x, *x_new);

    // accept time step
    x = x_new;
  }

  if (not cout_redirected)
    Dune::Logging::Logging::restoreCout();

  current_time() += dt;
  write_states();
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::get_data_handler(
  std::map<std::size_t, ConstState> states) const
{
  std::map<std::size_t, std::shared_ptr<DataHandler>> data;
  for (auto& [op, state] : states) {
    data[op] = std::make_shared<DataHandler>(
      *state.grid_function_space, *state.coefficients, _grid_view);
  }
  return data;
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::get_grid_function(
  const std::map<std::size_t, ConstState>& states,
  std::size_t comp) const -> std::shared_ptr<ComponentGridFunction>
{
  auto data = get_data_handler(states);
  auto& operator_config = _config.sub("operator");
  auto op_keys = operator_config.getValueKeys();
  std::sort(op_keys.begin(), op_keys.end());
  std::size_t op = operator_config.template get<std::size_t>(op_keys[comp]);
  std::size_t gfs_comp = 0;

  for (std::size_t i = 0; i < comp; i++)
    if (op == operator_config.template get<std::size_t>(op_keys[i]))
      gfs_comp++;

  const auto& data_comp = data.at(op);
  return std::make_shared<ComponentGridFunction>(
    data_comp->_lfs.child(gfs_comp), data_comp);
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::get_grid_function(std::size_t comp) const
  -> std::shared_ptr<ComponentGridFunction>
{
  return get_grid_function(const_states(), comp);
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::get_grid_functions(
  const std::map<std::size_t, ConstState>& states) const
  -> std::vector<std::shared_ptr<ComponentGridFunction>>
{
  std::size_t size = _config.sub("operator").getValueKeys().size();
  std::vector<std::shared_ptr<ComponentGridFunction>> grid_functions(size);
  for (std::size_t i = 0; i < size; i++) {
    grid_functions[i] = get_grid_function(states, i);
  }
  return grid_functions;
}

template<class Traits>
auto
ModelDiffusionReaction<Traits>::get_grid_functions() const
  -> std::vector<std::shared_ptr<ComponentGridFunction>>
{
  return get_grid_functions(const_states());
}
template<class Traits>
void
ModelDiffusionReaction<Traits>::write_states(
  const std::map<std::size_t, ConstState>& states) const
{
  if (!_sequential_writer) // skip write if not setup
    return;

  for (auto& [op, state] : _states) {
    auto& x = state.coefficients;
    auto& gfs = state.grid_function_space;

    auto etity_transformation = [&](auto e) {
      if constexpr (Concept::isMultiDomainGrid<Grid>() and
                    Concept::isSubDomainGrid<typename GV::Grid>())
        return _grid->multiDomainEntity(e);
      else
        return e;
    };
    using ET = decltype(etity_transformation);

    using Predicate = PDELab::vtk::DefaultPredicate;
    using Data = PDELab::vtk::DGFTreeCommonData<GFS, X, Predicate, GV, ET>;
    std::shared_ptr<Data> data =
      std::make_shared<Data>(*gfs, *x, _grid_view, etity_transformation);
    PDELab::vtk::OutputCollector<SW, Data> collector(*_sequential_writer, data);
    for (std::size_t k = 0; k < data->_lfs.degree(); k++)
      collector.addSolution(data->_lfs.child(k),
                            PDELab::vtk::defaultNameScheme());
  }
  _sequential_writer->write(current_time(), Dune::VTK::base64);
  _sequential_writer->vtkWriter()->clear();
}

template<class Traits>
void
ModelDiffusionReaction<Traits>::write_states() const
{
  write_states(const_states());
}

} // namespace Dune::Copasi

#endif // DUNE_COPASI_MODEL_DIFFUSION_REACTION_CC