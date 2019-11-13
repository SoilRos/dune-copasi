#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "grid_function_compare.hh"

#include <dune/copasi/common/enum.hh>
#include <dune/copasi/grid/multidomain_gmsh_reader.hh>
#include <dune/copasi/model/diffusion_reaction.cc>
#include <dune/copasi/model/diffusion_reaction.hh>

#include <dune/grid/multidomaingrid.hh>

#include <dune/grid/io/file/gmshreader.hh>
#include <dune/grid/uggrid.hh>

#include <dune/logging/logging.hh>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/common/parametertree.hh>
#include <dune/common/parametertreeparser.hh>

template<class Model>
void
model_compare(const Dune::ParameterTree& param, Model& model)
{
  auto do_step = [&]() {
    return Dune::FloatCmp::lt(model.current_time(), model.end_time());
  };

  // 1. create expression grid functions
  auto grid = model.states().begin()->second.grid;
  auto gv = grid->leafGridView();
  auto gf_expressions =
    model.get_muparser_expressions(param, "compare.expression", gv);

  auto compare = [&]() {
    auto setup_param = [&](auto var) {
      auto compare_config = param.sub("compare");
      Dune::ParameterTree param;

      if (compare_config.hasKey("l1_error." + var))
        param["l1_error"] = compare_config["l1_error." + var];
      if (compare_config.hasKey("l2_error." + var))
        param["l2_error"] = compare_config["l2_error." + var];
      if (compare_config.hasKey("linf_error." + var))
        param["linf_error"] = compare_config["linf_error." + var];
      return param;
    };

    // 2. get resulting grid functions
    auto gf_results = model.get_grid_functions();
    assert(gf_results.size() == gf_expressions.size());

    // 3. set expression grid functions to current time
    for (auto&& gf_expression : gf_expressions)
      gf_expression->set_time(model.current_time());

    // 4. compare expression vs resulting gf
    for (std::size_t i = 0; i < gf_results.size(); i++) {
      auto var = param.sub("operator").getValueKeys()[i];
      auto param_compare = setup_param(var);

      grid_function_compare(param_compare, *gf_expressions[i], *gf_results[i]);
    }
  };

  compare();

  while (do_step()) {
    model.step();

    compare();

    if (model.adaptivity_policy() != Dune::Copasi::AdaptivityPolicy::None)
      if (do_step()) {
        model.mark_grid();
        model.pre_adapt_grid();
        model.adapt_grid();
        model.post_adapt_grid();
      }
  }
}

int
main(int argc, char** argv)
{

  try {
    // initialize mpi
    auto& mpi_helper = Dune::MPIHelper::instance(argc, argv);
    auto comm = mpi_helper.getCollectiveCommunication();

    // Read and parse ini file
    if (argc != 2)
      DUNE_THROW(Dune::IOError, "Wrong number of arguments");
    const std::string config_filename = argv[1];

    Dune::ParameterTree config;
    Dune::ParameterTreeParser ptreeparser;
    ptreeparser.readINITree(config_filename, config);

    // initialize loggers
    Dune::Logging::Logging::init(comm, config.sub("logging"));

    using namespace Dune::Literals;
    auto log = Dune::Logging::Logging::logger(config);
    log.notice("Starting dune-copasi"_fmt);

    log.debug("Input config file '{}':"_fmt, config_filename);

    Dune::Logging::LoggingStream ls(false, log.indented(2));
    if (log.level() >= Dune::Logging::LogLevel::debug)
      config.report(ls);

    // create a multidomain grid
    //   here we use multidomain grids to be able to simulate
    //   each compartment individually and have similar input as in
    //   the multidomain case. However, it is possible to use
    //   ModelDiffusionReaction with single grids directly.
    constexpr int dim = 2;
    using HostGrid = Dune::UGGrid<dim>;
    using MDGTraits = Dune::mdgrid::DynamicSubDomainCountTraits<dim, 1>;
    using MDGrid = Dune::mdgrid::MultiDomainGrid<HostGrid, MDGTraits>;

    auto& grid_config = config.sub("grid");
    auto level = grid_config.get<int>("initial_level", 0);

    auto grid_file = grid_config.get<std::string>("file");

    auto [md_grid_ptr, host_grid_ptr] =
      Dune::Copasi::MultiDomainGmshReader<MDGrid>::read(grid_file, config);

    using Grid = typename MDGrid::SubDomainGrid;
    using GridView = typename Grid::Traits::LeafGridView;

    log.debug("Applying global refinement of level: {}"_fmt, level);
    md_grid_ptr->globalRefine(level);

    auto& model_config = config.sub("model");
    auto& compartments_map = model_config.sub("compartments");
    int order = model_config.get<int>("order");

    if (not model_config.hasKey("begin_time"))
      DUNE_THROW(Dune::RangeError, "Key 'model.begin_time' is missing");
    if (not model_config.hasKey("end_time"))
      DUNE_THROW(Dune::RangeError, "Key 'model.end_time' is missing");
    if (not model_config.hasKey("time_step"))
      DUNE_THROW(Dune::RangeError, "Key 'model.time_step' is missing");

    if (compartments_map.getValueKeys().size() > 1)
      log.warn(
        "This executable solve models for each compartment individually. If "
        "some coupling is exected on the interface between compartments, use "
        "'dune_copasi_md' executable for multidomain models"_fmt);
    // @todo check coupling at interface and refuse to compute coupled models

    // solve individual proble for each compartment
    for (auto&& compartment : compartments_map.getValueKeys()) {
      log.info("Running model for '{}' compartment"_fmt, compartment);

      // get subdomain grid as a shared pointer
      int domain = compartments_map.template get<int>(compartment);
      std::shared_ptr<Grid> grid_ptr =
        Dune::stackobject_to_shared_ptr(md_grid_ptr->subDomain(domain));

      // get a model configuration parameter tree for this compartment
      auto compartment_config = model_config.sub(compartment);

      // add missing keywords for individual models

      // ...time keys
      compartment_config["begin_time"] = model_config["begin_time"];
      compartment_config["end_time"] = model_config["end_time"];
      compartment_config["time_step"] = model_config["time_step"];

      // ...data keys
      if (model_config.hasSub("data")) {
        const auto& data_config = model_config.sub("data");
        for (auto&& key : data_config.getValueKeys())
          compartment_config["data." + key] = data_config[key];
      }

      if (order == 1) {
        constexpr int Order = 1;
        using ModelTraits =
          Dune::Copasi::ModelPkDiffusionReactionTraits<Grid, GridView, Order>;
        Dune::Copasi::ModelDiffusionReaction<ModelTraits> model(
          grid_ptr, compartment_config);
        model_compare(compartment_config, model);
      } else if (order == 2) {
        constexpr int Order = 2;
        using ModelTraits =
          Dune::Copasi::ModelPkDiffusionReactionTraits<Grid, GridView, Order>;
        Dune::Copasi::ModelDiffusionReaction<ModelTraits> model(
          grid_ptr, compartment_config);
        model_compare(compartment_config, model);
      } else {
        DUNE_THROW(Dune::IOError,
                   "Finite element order "
                     << order << " is not supported by dune-copasi");
      }
    }

  } catch (Dune::Exception& e) {
    std::cerr << "Dune reported error: " << e << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Unknown exception thrown!" << std::endl;
    return 1;
  }
  return 0;
}
