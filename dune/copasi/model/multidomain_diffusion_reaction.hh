#ifndef DUNE_COPASI_MODEL_MULTIDOMAIN_DIFFUSION_REACTION_HH
#define DUNE_COPASI_MODEL_MULTIDOMAIN_DIFFUSION_REACTION_HH

#include <dune/copasi/common/enum.hh>
#include <dune/copasi/concepts/grid.hh>
#include <dune/copasi/grid/multidomain_entity_transformation.hh>
#include <dune/copasi/model/base.hh>
#include <dune/copasi/model/diffusion_reaction.cc>
#include <dune/copasi/model/diffusion_reaction.hh>
#include <dune/copasi/model/multidomain_local_operator.hh>

#include <dune/pdelab/backend/istl.hh>
#include <dune/pdelab/backend/istl/novlpistlsolverbackend.hh>
#include <dune/pdelab/gridfunctionspace/dynamicpowergridfunctionspace.hh>
#include <dune/pdelab/gridfunctionspace/vtk.hh>

#include <dune/common/parametertree.hh>

#include <memory>

namespace Dune::Copasi {

/**
 * @brief      Traits for diffusion reaction models in multigrid domains
 *
 * @tparam     G         Grid
 * @tparam     FEMorder  Order of the finite element method
 * @tparam     OT        PDELab ordering tag
 * @tparam     JM        Jacobian method
 */
template<class G,
         int FEMorder = 1,
         class OT = PDELab::EntityBlockedOrderingTag,
         JacobianMethod JM = JacobianMethod::Analytical>
struct ModelMultiDomainPkDiffusionReactionTraits
{
  using Grid = G;

  // Check grid template
  static_assert(Concept::isMultiDomainGrid<Grid>(),
                "Provided grid type is not a multidomain grid");

  static_assert(Concept::isSubDomainGrid<typename Grid::SubDomainGrid>());

  using OrderingTag = OT;

  static constexpr JacobianMethod jacobian_method = JM;

  using SubModelTraits =
    ModelPkDiffusionReactionTraits<Grid,
                                   typename Grid::SubDomainGrid::LeafGridView,
                                   FEMorder,
                                   OrderingTag,
                                   jacobian_method>;
};


template<class G,
         int FEMorder = 1,
         class OT = PDELab::EntityBlockedOrderingTag,
         JacobianMethod JM = JacobianMethod::Analytical>
struct ModelMultiDomainP0PkDiffusionReactionTraits
{
  using Grid = G;

  // Check grid template
  static_assert(Concept::isMultiDomainGrid<Grid>(),
                "Provided grid type is not a multidomain grid");

  static_assert(Concept::isSubDomainGrid<typename Grid::SubDomainGrid>());

  using OrderingTag = OT;

  static constexpr JacobianMethod jacobian_method = JM;

  using SubModelTraits =
    ModelP0PkDiffusionReactionTraits<Grid,
                                   typename Grid::SubDomainGrid::LeafGridView,
                                   FEMorder,
                                   OrderingTag,
                                   jacobian_method>;
};

/**
 * @brief      Class for diffusion-reaction models in multigrid domains.
 *
 * @tparam     Traits  Class that define static policies on the model
 */
template<class Traits_>
class ModelMultiDomainDiffusionReaction : public ModelBase
{
public:
  using Traits = Traits_;

private:
  using Grid = typename Traits::Grid;

  using OT = typename Traits::OrderingTag;

  static constexpr JacobianMethod JM = Traits::jacobian_method;

  using SubDomainGridView = typename Grid::SubDomainGrid::LeafGridView;

  using SubModel = ModelDiffusionReaction<typename Traits::SubModelTraits>;

  //! Grid view
  using GridView = typename Grid::LeafGridView;
  using GV = GridView;

  //! Domain field
  using DF = typename Grid::ctype;

  //! Range field
  using RF = double;

  //! Constraints builder
  using CON = PDELab::ConformingDirichletConstraints;

  //! Leaf vector backend
  using LVBE = PDELab::ISTL::VectorBackend<>;

  //! Leaf grid function space
  using LGFS = typename SubModel::LGFS;

  //! SubDomain grid function space
  using SDGFS = typename SubModel::GFS;

  //! Vector backend
  using VBE = typename LGFS::Traits::Backend;
  using GFS = PDELab::DynamicPowerGridFunctionSpace<SDGFS, VBE>;

  //! Coefficient vector
  using X = PDELab::Backend::Vector<GFS, RF>;

  //! Constraints container
  using CC = typename GFS::template ConstraintsContainer<RF>::Type;

  //! Model state structure
  using State = Dune::Copasi::ModelState<Grid, GFS, X>;

  //! Constant model state structure
  using ConstState = Dune::Copasi::ConstModelState<Grid, GFS, X>;

  //! Coefficients mapper
  using CM = Dune::Copasi::MultiDomainModelCoefficientMapper<ConstState>;

  //! Local operator
  using LOP = LocalOperatorMultiDomainDiffusionReaction<
    Grid,
    typename Traits::SubModelTraits::template LocalOperator<CM>,
    JM>;

  //! Temporal local operator
  using TLOP = TemporalLocalOperatorMultiDomainDiffusionReaction<
    Grid,
    typename Traits::SubModelTraits::template TemporalLocalOperator<CM>,
    JM>;

  //! Matrix backend
  using MBE = Dune::PDELab::ISTL::BCRSMatrixBackend<>;

  //! Spatial grid operator
  using GOS =
    Dune::PDELab::GridOperator<GFS, GFS, LOP, MBE, RF, RF, RF, CC, CC>;

  //! Temporal grid operator
  using GOT =
    Dune::PDELab::GridOperator<GFS, GFS, TLOP, MBE, RF, RF, RF, CC, CC>;

  //! Instationary grid operator
  using GOI = Dune::PDELab::OneStepGridOperator<GOS, GOT>;

  //! Linear solver backend
  using LS = Dune::PDELab::ISTLBackend_NOVLP_BCGS_SSORk<GOI>;

  //! Nonlinear solver
  using NLS = Dune::PDELab::Newton<GOI, LS, X>;

  //! Time stepping parameter
  using TSP = Dune::PDELab::TimeSteppingParameterInterface<double>;

  //! One step method
  using OSM = Dune::PDELab::OneStepMethod<RF, GOI, NLS, X, X>;

  //! Writer
  using W = Dune::VTKWriter<SubDomainGridView>;

  //! Sequential writer
  using SW = Dune::VTKSequenceWriter<SubDomainGridView>;

  //! Entity transformation between grids
  using EntityTransformation =
    Dune::Copasi::MultiDomainEntityTransformation<Grid>;

  using DataHandler =
    PDELab::vtk::DGFTreeCommonData<GFS,
                                   X,
                                   PDELab::vtk::DefaultPredicate,
                                   SubDomainGridView,
                                   EntityTransformation>;

  using ComponentLFS =
    typename PDELab::LocalFunctionSpace<GFS>::ChildType::ChildType;

  using ComponentGridFunction = PDELab::vtk::
    DGFTreeLeafFunction<ComponentLFS, DataHandler, SubDomainGridView>;

public:
  /**
   * @brief      Constructs a new instance.
   *
   * @param[in]  grid    The grid
   * @param[in]  config  The configuration
   */
  ModelMultiDomainDiffusionReaction(
    std::shared_ptr<Grid> grid,
    const Dune::ParameterTree& config,
    ModelSetupPolicy setup_policy = ModelSetupPolicy::All);

  /**
   * @brief      Destroys the object.
   */
  ~ModelMultiDomainDiffusionReaction();

  /**
   * @brief      Get mutable model states
   *
   * @return     Model states
   */
  std::map<std::size_t, State> states() { return _states; }

  /**
   * @brief      Get constat model states
   *
   * @return     Constant model states
   */
  std::map<std::size_t, ConstState> const_states() const
  {
    return const_states(_states);
  }

  /**
   * @brief      Get constat model states
   *
   * @param[in]  states  A map to mutable states
   *
   * @return     Constant model states
   */
  std::map<std::size_t, ConstState> const_states(
    const std::map<std::size_t, State>& states) const
  {
    std::map<std::size_t, ConstState> const_states(states.begin(),
                                                   states.end());
    return const_states;
  }

  /**
   * @brief      Get the model states
   *
   * @return     Model states
   */
  std::map<std::size_t, ConstState> states() const { return const_states(); }

  /**
   * @brief      Sets the initial state of the model
   *
   * @param[in]  model_config  A parameter tree with 'initial' and optionally
   * 'data' subsections
   */
  template<class GFGridView>
  static auto get_muparser_initial(const ParameterTree& model_config,
                                   const GFGridView& gf_grid_view , bool compile = true);

  /**
   * @brief      Sets the initial state of the model
   * @details    The input vector of vectors should have the same size as the number of
   * domains variables in the model, and each vector for each subdomain has to have the
   * same size as the number of variables in the compartment. Additionally, variables
   * will be indepreted aphabetically
   * accodingly to the name set to othe input sections (e.g. 'model.<compartment>.diffusion'
   * section).
   *
   * @tparam     GF       A valid PDELab grid functions (see
   * @Concepts::PDELabGridFunction)
   * @param[in]  initial  Vector of vecotrs of grid functions, one for each variable
   */
  template<class GF>
  void set_initial(const std::vector<std::vector<std::shared_ptr<GF>>>& initial);

  /**
   * @brief      Setup function
   * @details    This class sets up the model to have completely defined the
   *             requested feature in the policy
   *
   * @param[in]  setup_policy  The setup policy
   */
  void setup(ModelSetupPolicy setup_policy = ModelSetupPolicy::All);

  /**
   * @copydoc ModelBase::suggest_timestep
   */
  void suggest_timestep(double dt) override;

  /**
   * @copydoc ModelBase::step
   */
  void step() override;

  /**
   * @brief      Gets a grid function for a given component and a sub domain.
   * @todo       Make this function operate on constant states
   *
   * @param[in]  states  The model states
   * @param[in]  domain  The domain
   * @param[in]  comp    The component
   *
   * @return     The grid function.
   */
  auto get_grid_function(const std::map<std::size_t, State>& states,
                         std::size_t domain,
                         std::size_t comp) const;

  auto get_grid_function(std::size_t domain, std::size_t comp) const;

  auto get_grid_functions(const std::map<std::size_t, State>& states) const;

  auto get_grid_functions() const;

protected:
  void setup_grid_function_spaces();
  void setup_coefficient_vectors();
  void setup_constraints();
  auto setup_local_operator(std::size_t) const;
  void setup_local_operators();
  void setup_grid_operators();
  void setup_solvers();
  void setup_vtk_writer();
  void write_states() const;

  void update_data_handler();
  auto get_data_handler(std::map<std::size_t, State>) const;

private:
  using ModelBase::_logger;
  Logging::Logger _solver_logger;

  ParameterTree _config;
  GV _grid_view;

  std::vector<std::shared_ptr<SW>> _sequential_writer;

  std::map<std::size_t, State> _states;
  std::shared_ptr<Grid> _grid;

  std::vector<std::map<std::size_t, std::shared_ptr<DataHandler>>> _data;

  std::map<std::size_t, std::unique_ptr<CC>> _constraints;
  std::map<std::size_t, std::shared_ptr<LOP>> _local_operators;
  std::map<std::size_t, std::shared_ptr<TLOP>> _temporal_local_operators;
  std::map<std::size_t, std::shared_ptr<GOS>> _spatial_grid_operators;
  std::map<std::size_t, std::shared_ptr<GOT>> _temporal_grid_operators;
  std::map<std::size_t, std::shared_ptr<GOI>> _grid_operators;
  std::map<std::size_t, std::shared_ptr<LS>> _linear_solvers;
  std::map<std::size_t, std::shared_ptr<NLS>> _nonlinear_solvers;
  std::map<std::size_t, std::shared_ptr<TSP>> _time_stepping_methods;
  std::map<std::size_t, std::shared_ptr<OSM>> _one_step_methods;

  std::size_t _domains;
  std::size_t _dof_per_component;
};

} // namespace Dune::Copasi

#endif // DUNE_COPASI_MODEL_MULTIDOMAIN_DIFFUSION_REACTION_HH