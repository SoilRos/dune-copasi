[![Build Status](https://gitlab.dune-project.org/copasi/dune-copasi/badges/master/pipeline.svg)](https://gitlab.dune-project.org/copasi/dune-copasi/pipelines)
[![Build Status](https://travis-ci.org/SoilRos/dune-copasi.svg?branch=master)](https://travis-ci.org/SoilRos/dune-copasi)
[![Build status](https://ci.appveyor.com/api/projects/status/6605joy2w17qvca8/branch/master?svg=true)](https://ci.appveyor.com/project/SoilRos/dune-copasi/history)

# dune-copasi

Solver for reaction-diffusion systems in multiple compartments

 * Solve a reaction-diffusion system for each comartment
 * Each compartment may have different system with different number of variables
 * Neumann flux at the interface of compartments for variables with
   the same name on the two compartments
 * Easy to modify configuration file
 * Solved using the finite element method
 * Output in the VTK format

This project is made under the umbrella of the 
[*Distributed and Unified Numerics Environment* `DUNE`](https://www.dune-project.org/) and the
[*Biochemical System Simulator* `COPASI`](http://copasi.org/). 
Altought the rationale of the design is always driven by biochemical process (e.g. cell biology), 
this software is not limited to this scope and can be used for other processes involving reaction-diffusion system.

## Graphical User Interface for SMBL files

For those working in bio-informatics there exist a grafical user interface for SMBL files!
The GUI is able to convert non-spatial SBML models of bio-chemical reactions into 
2d spatial models, and to simulate them with `dune-copasi`.

https://github.com/lkeegan/spatial-model-editor

## Installation

This requires that you have installed the following packages before the actual installation of `dune-copasi`.

| Software | Version/Branch | Comments |
| ---------| -------------- | -------- |
| [CMake](https://cmake.org/) | 3.1 |
| C++ compiler | [C++17](https://en.wikipedia.org/wiki/List_of_compilers#C++_compilers) | 
| [libTIFF](http://www.libtiff.org/) | 3.6.1 |
| [muParser](https://beltoforion.de/article.php?a=muparser) | 2.2.5 |
| [dune-common](https://gitlab.dune-project.org/santiago.ospina/dune-common) | support/dune-copasi | https://gitlab.dune-project.org/santiago.ospina/dune-common
| [dune-logging](https://gitlab.dune-project.org/staging/dune-logging) | master | https://gitlab.dune-project.org/staging/dune-logging
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry) | master | https://gitlab.dune-project.org/core/dune-geometry
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid) | master | https://gitlab.dune-project.org/core/dune-grid
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid) | master | https://gitlab.dune-project.org/staging/dune-uggrid
| [dune-istl](https://gitlab.dune-project.org/core/dune-istl) | master | https://gitlab.dune-project.org/core/dune-istl
| [dune-localfunctions](https://gitlab.dune-project.org/core/dune-localfunctions) | master | https://gitlab.dune-project.org/core/dune-localfunctions
| [dune-functions](https://gitlab.dune-project.org/staging/dune-functions) | master | https://gitlab.dune-project.org/staging/dune-functions
| [dune-typetree](https://gitlab.dune-project.org/santiago.ospina/dune-typetree) | support/dune-copasi | https://gitlab.dune-project.org/santiago.ospina/dune-typetree
| [dune-pdelab](https://gitlab.dune-project.org/santiago.ospina/dune-pdelab) | support/dune-copasi | https://gitlab.dune-project.org/santiago.ospina/dune-pdelab
| [dune-multidomaingrid](https://gitlab.dune-project.org/santiago.ospina/dune-multidomaingrid) | support/dune-copasi | https://gitlab.dune-project.org/santiago.ospina/dune-multidomaingrid

The first four can be obtained by your prefered package manager in unix-like operating systems. e.g.

```bash
# if you are in a debian/ubuntu OS
apt update
apt install cmake gcc g++ libtiff-dev libmuparser-dev git

# if you are in a macOS
xcode-select --install # Apple Command Line Tools
brew update
brew install cmake gcc libtiff muparser git
```

Now, the dune modules (including `dune-copasi`) can be all checkout in a same folder and be installed in one go. 

```bash
# prepare a folder to download and build dune modules
mkdir ~/dune-modules && cd ~/dune-modules

# fetch dependencies & dune-copasi in ~/dune-modules folder
git clone https://gitlab.dune-project.org/santiago.ospina/dune-common
git clone https://gitlab.dune-project.org/staging/dune-logging
git clone https://gitlab.dune-project.org/core/dune-geometry
git clone https://gitlab.dune-project.org/core/dune-grid
git clone https://gitlab.dune-project.org/staging/dune-uggrid
git clone https://gitlab.dune-project.org/core/dune-istl
git clone https://gitlab.dune-project.org/core/dune-localfunctions
git clone https://gitlab.dune-project.org/staging/dune-functions
git clone https://gitlab.dune-project.org/santiago.ospina/dune-typetree
git clone https://gitlab.dune-project.org/santiago.ospina/dune-pdelab
git clone https://gitlab.dune-project.org/santiago.ospina/dune-multidomaingrid
git clone https://gitlab.dune-project.org/copasi/dune-copasi

# configure and build dune modules
./dune-common/bin/dunecontrol make all

# install dune-copasi (this operation may requiere sudo)
./dune-common/bin/dunecontrol --only=dune-copasi bexec make install

# if you do not want to install dune-copasi system wide, you can set
# the CMAKE_INSTALL_PREFIX  to a non restricted folder
# see https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.htmlZ
```

For further info on dune module installation process, please check out 
the [dune-project web page](https://www.dune-project.org/doc/installation/)

## Usage 

If you installed `dune-copasi` system wide, you should be able to call the program
`dune_copasi` from your command line accompained with a configuration file.

```bash
dune_copasi config.ini
```

* TODO: Add an example of `config.ini`.
* TODO: Add licence