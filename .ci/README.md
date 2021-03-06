[![Build Status](https://gitlab.dune-project.org/copasi/dune-copasi/badges/master/pipeline.svg)](https://gitlab.dune-project.org/copasi/dune-copasi/pipelines)
[![Build Status](https://travis-ci.org/dune-copasi/dune-copasi.svg?branch=master)](https://travis-ci.org/dune-copasi/dune-copasi)
[![Build Status](https://ci.appveyor.com/api/projects/status/e7w7u5dt50kue5sb/branch/master?svg=true)](https://ci.appveyor.com/project/SoilRos/dune-copasi-3gv18/branch/master)

## How does this CI works?

We have 3 Continous Integration services:
  - [GitLab](https://docs.gitlab.com/ee/ci/)
  - [Travis](https://travis-ci.org/)
  - [AppVeyor](https://www.appveyor.com/)

The reason for this is because only one could not tackle all three platforms we
want to reach. That is:
  - Linux
  - macOS
  - Windows

Sice the last two CI are better ingegrated with GitHub, we have set a repository
there which mirrors all modifications made in the main repository.

  - Main repository: https://gitlab.dune-project.org/copasi/dune-copasi
  - Mirror repository: https://github.com/dune-copasi/dune-copasi

The main idea here is that they, the different CI, follow almost the same
instructions in the different stages. In all the cases, the scripts expect
to have defined `DUNECONTROL` and `DUNE_OPTIONS_FILE` variables for running
`dunecontrol` and for the configuration options.

### Stage: Setup

In this stage, we configure and install all the dependencies that `dune-copasi`
requires. In this case, Travis and Appveyor use the script
[`setup.sh`](setup.sh) while gitlab uses the dependencies docker image in the
container registry. The latter only updated for the `master` branch by the
docker script [`dependencies.dockerfile`](../docker/dependencies.dockerfile).
This makes the GitLab CI run faster as all the dependencies are already
installed.

### Stage: Build

This stage builds `dune-copasi` following the [`build.sh`](build.sh) script
which in turs run the `dunecontrol` command for the project.
This stage always should be run.

### Stage: Unit Tests

This stage builds and runs the unit tests by running the
[`unit_tests.sh`](unit_tests.sh) script. `ctests` runs all tests with the tag
`unit`. This stage always should be run.

### Stage: System Tests

This stage runs all the system tests tagged with `DUNE_SYSTEMTEST` when running
the [`system_tests.sh`](system_tests.sh) script. Since most of the system tests
are generated by `dune-testools`, which is optional, this stage will only make
sense when `dune-testools` is installed in the setup stage. Currently, this i 
not feasible for windows due to the python environment setup.


## CI Link & Badge
For getting them replace `<branch>` for the specific branch you want:

### GitLab CI
  - Link: https://gitlab.dune-project.org/copasi/dune-copasi/pipelines
  - Badge: [https://gitlab.dune-project.org/copasi/dune-copasi/badges/`<branch>`/pipeline.svg](https://gitlab.dune-project.org/copasi/dune-copasi/badges/master/pipeline.svg)
### Travis
  - Link: https://travis-ci.org/SoilRos/dune-copasi
  - Bagage [https://travis-ci.org/dune-copasi/dune-copasi.svg?branch=`<branch>`](https://travis-ci.org/dune-copasi/dune-copasi/branches)
### AppVeyor
  - Link: [https://ci.appveyor.com/project/SoilRos/dune-copasi/branch/`<branch>`](https://ci.appveyor.com/project/SoilRos/dune-copasi-3gv18/branch/master)
  - Badge: [https://ci.appveyor.com/api/projects/status/e7w7u5dt50kue5sb/branch/`<branch>`?svg=true](https://ci.appveyor.com/api/projects/status/e7w7u5dt50kue5sb/branch/master?svg=true)


