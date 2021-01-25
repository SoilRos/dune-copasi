---
id: install_use
title: Installation and Usage
sidebar_label: Install & Use
---

import Tabs from '@theme/Tabs';
import TabItem from '@theme/TabItem';

There are two forms to use `DuneCopasi`. The first and easiest one is using a
[Docker Container](https://www.docker.com/) where the software is boundled such
that no installation other than docker is required. On the second one, the
compilation of the source code is compulsory.

## Docker Usage

In this mode, there is no need to fullfil specific requirements other than
those for docker installation.

### Install Docker
First, get and install Docker following the
[docker installation instructions](https://docs.docker.com/get-docker/).

### Prepare working directory

To be able to share data between your operating system and within the docker
image create a folder with read/write/execute rights to _any_ user:

```bash
mkdir -m 777 dunecopasi && cd dunecopasi
```

This working directory will be accessible to your text editor and paraview as
well as to the `dune-copasi-md` executable inside the docker container. Thus,
move or create your configuration files into it at will.

```
nano config.ini
# set-up/write ini file for dune-copasi...
```

### Run the program

Then, you may use the latest stable container by pulling/runing it from our
[GitLab registry](https://gitlab.dune-project.org/copasi/dune-copasi/container_registry)
To run the `dune-copasi-md` executable from the container with a configuration
file `config.ini`, execute the following command on the terminal:

```bash
docker run -v $PWD:/dunecopasi \
  registry.dune-project.org/copasi/dune-copasi/dune-copasi:latest \
  dune-copasi-md config.ini
```

The results of those computations will be written on current
directory as mentioned above. For more information about running docker images,
visit the `docker run` [documentation](https://docs.docker.com/engine/reference/run/).

## Manual Installation

Locally building and installing `DuneCopasi` requires to first obtain the
dependencies, then compile the dependent dune modules and finally installing on
a final directory.

### Dependencies

The following list of software is required to install and use `DuneCopasi`:

:::info
Notice that some required dune modules are forks of original reopsitories and
are placed under the [COPASI namespace](https://gitlab.dune-project.org/copasi/)
on the [DUNE GitLab](https://gitlab.dune-project.org/).
:::

| Software | Version/Branch |
| ---------| -------------- |
| [CMake](https://cmake.org/)                                                                 | >= 3.1 |
| C++ compiler  | >= [C++17](https://en.wikipedia.org/wiki/List_of_compilers#C++_compilers) |
| [libTIFF](http://www.libtiff.org/)                                                          | >= 3.6.1 |
| [muParser](https://beltoforion.de/article.php?a=muparser)                                   | >= 2.2.5 |
| [dune-common](https://gitlab.dune-project.org/copasi/dune-common)                           | == 2.7 |
| [dune-geometry](https://gitlab.dune-project.org/core/dune-geometry)                         | == 2.7 |
| [dune-grid](https://gitlab.dune-project.org/core/dune-grid)                                 | == 2.7 |
| [dune-uggrid](https://gitlab.dune-project.org/staging/dune-uggrid)                          | == 2.7 |
| [dune-istl](https://gitlab.dune-project.org/core/dune-istl)                                 | == 2.7 |
| [dune-localfunctions](https://gitlab.dune-project.org/core/dune-localfunctions)             | == 2.7 |
| [dune-functions](https://gitlab.dune-project.org/staging/dune-functions)                    | == 2.7 |
| [COPASI/dune-logging](https://gitlab.dune-project.org/copasi/dune-logging)                  | `support/dune-copasi-latest` |
| [COPASI/dune-typetree](https://gitlab.dune-project.org/copasi/dune-typetree)                | `support/dune-copasi-latest` |
| [COPASI/dune-pdelab](https://gitlab.dune-project.org/copasi/dune-pdelab)                    | `support/dune-copasi-latest` |
| [COPASI/dune-multidomaingrid](https://gitlab.dune-project.org/copasi/dune-multidomaingrid)  | `support/dune-copasi-latest` |

### Installation

The first four can be obtained by your prefered package manager in unix-like operating systems. e.g.

<Tabs
  groupId="operating-systems"
  defaultValue="win"
  values={[
      {label: 'Debian/Ubuntu', value: 'deb', },
      {label: 'macOS', value: 'mac', },
    ]
  }>

  <TabItem value="deb">

```bash
apt update
apt install cmake gcc g++ libtiff-dev libmuparser-dev git
```

  </TabItem>
  <TabItem value="mac">

```bash
# Apple Command Line Tools and Brew assumed to be available
brew update
brew install cmake gcc libtiff muparser git
```

  </TabItem>
</Tabs>

The required `DUNE` modules (including `DuneCopasi`) can be obtained via internet
by using [`git`](https://git-scm.com/). For smooth installation, is better place
all the dune modules within the same directory.

```bash
# prepare a folder to download and build dune modules
mkdir ~/dune-modules && cd ~/dune-modules

# fetch dependencies & dune-copasi in ~/dune-modules folder
git clone -b releases/2.7 https://gitlab.dune-project.org/core/dune-common
git clone -b releases/2.7 https://gitlab.dune-project.org/core/dune-geometry
git clone -b releases/2.7 https://gitlab.dune-project.org/core/dune-grid
git clone -b releases/2.7 https://gitlab.dune-project.org/staging/dune-uggrid
git clone -b releases/2.7 https://gitlab.dune-project.org/core/dune-istl
git clone -b releases/2.7 https://gitlab.dune-project.org/core/dune-localfunctions
git clone -b releases/2.7 https://gitlab.dune-project.org/staging/dune-functions
git clone -b support/dune-copasi-latest --recursive https://gitlab.dune-project.org/copasi/dune-logging
git clone -b support/dune-copasi-latest https://gitlab.dune-project.org/copasi/dune-typetree
git clone -b support/dune-copasi-latest https://gitlab.dune-project.org/copasi/dune-pdelab
git clone -b support/dune-copasi-latest https://gitlab.dune-project.org/copasi/dune-multidomaingrid
git clone -b latest https://gitlab.dune-project.org/copasi/dune-copasi
```

Then build and install the `DUNE` modules with the `dunecontrol` script:
```bash

# add custom build options...
# set install directory to /opt/dune/
echo 'CMAKE_FLAGS+=" -DCMAKE_INSTALL_PREFIX=/opt/dune"' >> dune.opts

# configure and build dune modules
./dune-common/bin/dunecontrol --opts=dune.opts all

# install binaries and libraries into /opt/dune/
./dune-common/bin/dunecontrol bexec make install

# remove source and build files
cd ~ && rm -r ~/dune-modules

# include dune binaries into your path
echo export PATH="/opt/dune/bin:$PATH" >> $HOME/.bashrc
```

For further info on dune module installation process, please check out
the [dune-project web page](https://www.dune-project.org/doc/installation/).

### Run the program

Now, you should be able to call the program `dune-copasi-md` from your command
line accompained with a configuration file:

```bash
dune-copasi-md config.ini
```

To find out the appropiated contents on the configuration file, check out
the [Parameter Tree](param_tree.md) documentation.

## Importing CMake targets

In order to use the C++ interface, you must have compiled and (optionally)
installed all the dune packages from source. Then, you will be able to import
and link the CMake targets for `dune-copasi` by simply calling the following
commands in CMake:

```cmake
# ...
find_package(dune-copasi IMPORTED)
target_link_libraries(my_app PRIVATE dune::dune-copasi)
# ...
```

If the `CMAKE_INSTALL_PREFIX` options was used on installation, be sure to also
include it on the app build settings:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/dune /path/to/app/source/
```
