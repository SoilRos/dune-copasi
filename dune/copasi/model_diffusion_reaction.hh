#ifndef DUNE_COPASI_MODEL_DIFFUSION_REACTION_HH
#define DUNE_COPASI_MODEL_DIFFUSION_REACTION_HH

#include <dune/copasi/util_meta.hh>
#include <dune/copasi/model_base.hh>
#include <dune/copasi/model_state.hh>
#include <dune/copasi/local_operator.hh>

#include <dune/pdelab/finiteelementmap/qkfem.hh>
#include <dune/pdelab/constraints/conforming.hh>
#include <dune/pdelab/gridfunctionspace/gridfunctionspace.hh>
#include <dune/pdelab/backend/istl.hh>
#include <dune/pdelab/gridoperator/gridoperator.hh>
#include <dune/pdelab/gridoperator/onestep.hh>
#include <dune/pdelab/newton/newton.hh>

#include <dune/grid/uggrid.hh>

#include <memory>

namespace Dune::Copasi {

/**
 * @brief      Class for diffusion-reaction models.
 *
 * @tparam     components  Number of components
 */
template<int components, class Param>
class ModelDiffusionReaction : public ModelBase {

  //! World dimension
  static constexpr int dim = 2;

  //! Grid type
  using Grid = Dune::UGGrid<dim>;

  //! Grid view
  using GV = typename Grid::LeafGridView;

  //! Domain field
  using DF = typename Grid::ctype;

  //! Range field
  using RF = double;

  //! Finite element map
  using FEM = PDELab::QkLocalFiniteElementMap<GV,DF,RF,1>;

  //! Constraints builder
  using CON = PDELab::ConformingDirichletConstraints;

  //! Leaf vector backend
  using LVBE = PDELab::ISTL::VectorBackend<>;

  //! Leaf grid function space
  using LGFS = PDELab::GridFunctionSpace<GV,FEM,CON,LVBE>;

  //! Vector backend
  using VBE = LVBE;

  //! Ordering tag
  using OrderingTag = PDELab::LexicographicOrderingTag;

  //! Grid function space
  using GFS = PDELab::PowerGridFunctionSpace<LGFS,components,VBE,OrderingTag>;

  //! Coefficient vector
  using X = PDELab::Backend::Vector<GFS,RF>;

  //! Constraints container
  using CC = typename GFS::template ConstraintsContainer<RF>::Type;

  //! Local operator
  using LOP = LocalOperatorDiffusionReaction<Param,FEM,4>;

  //! Temporal local operator 
  using TLOP = TemporalLocalOperatorDiffusionReaction<FEM,components>;

  //! Matrix backend
  using MBE = Dune::PDELab::ISTL::BCRSMatrixBackend<>;

  //! Spatial grid operator
  using GOS = Dune::PDELab::GridOperator<GFS,GFS,LOP,MBE,RF,RF,RF,CC,CC>;

  //! Temporal grid operator
  using GOT = Dune::PDELab::GridOperator<GFS,GFS,TLOP,MBE,RF,RF,RF,CC,CC>;

  //! Instationary grid operator
  using GOI = Dune::PDELab::OneStepGridOperator<GOS,GOT>;

  //! Linear solver backend
  using LS = Dune::PDELab::ISTLBackend_SEQ_BCGS_SSOR;

  //! Nonlinear solver
  using NLS = Dune::PDELab::Newton<GOI,LS,X>;

  //! One step method
  using OMS = Dune::PDELab::OneStepMethod<RF,GOI,NLS,X,X>;

  //! Writer
  using W = Dune::VTKWriter<GV>;

  //! Sequential writer
  using SW = Dune::VTKSequenceWriter<GV>;

public:

  //! Model state structure
  using ModelState = Dune::Copasi::ModelState<Grid,GFS,X>;

  //! Constant model state structure
  using ConstModelState = Dune::Copasi::ModelState<const Grid,const GFS,const X>;

  //! Model parameterization
  using Parameterization = Param;

  /**
   * @brief      Constructs the model
   *
   * @param[in]  grid    The grid
   * @param[in]  config  The configuration file
   */
  ModelDiffusionReaction(std::shared_ptr<Grid> grid, 
                         const Dune::ParameterTree& config);

  /**
   * @brief      Destroys the mode
   */
  ~ModelDiffusionReaction();

  /**
   * @brief      Suggest a time step to the model.
   *
   * @param[in]  dt    Suggestion for the internal time step of the model.
   */
  void suggest_timestep(double dt);

  /**
   * @brief      Performs one steps in direction to end_time().
   * @details    The time-step should never result on a bigger step than the one
   *             suggested in suggest_timestep().
   */
  void step();

  /**
   * @brief      Get the model state 
   *
   * @return     Model state
   */
  ModelState state();

  /**
   * @brief      Get the model state 
   *
   * @return     Model state
   */
  ConstModelState state() const;

  /**
   * @brief      Sets the state of the model
   *
   * @param[in]  input_state  The state to set in the model
   *
   * @tparam     T            Type of valid input states. Valid states are:
   *                          * Arithmetic values: Set all components with the
   *                            same value everywhere in the domain
   *                          * Field vector: Set each component with the values
   *                            in the field vector everywhere in the domain
   *                          * [WIP] PDELab callable: A lambda function which
   *                            that returns a field vector with the components
   *                            state for every position in the domain
   *                          * [WIP] PDELab grid function: A function following
   *                            the PDELab grid function interface
   */
  template<class T>
  void set_state(const T& input_state);

protected:

  /**
   * @brief      Setup operator for next time step
   */
  void operator_setup();

  using   ModelBase::_logger;

private:
  GV                                _grid_view;
  ModelState                        _state;
  std::shared_ptr<FEM>              _finite_element_map;
  std::unique_ptr<CC>               _constraints;
  std::shared_ptr<Parameterization> _parameterization;
};

} // Dune::Copasi namespace

#endif // DUNE_COPASI_MODEL_DIFFUSION_REACTION_HH