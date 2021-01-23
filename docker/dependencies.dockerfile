ARG BASE_IMAGE=registry.dune-project.org/docker/ci/ubuntu:18.04
FROM ${BASE_IMAGE}
ARG DUNECI_PARALLEL=2

ARG TOOLCHAIN="clang-6-17"
RUN ln -s /duneci/toolchains/${TOOLCHAIN} /duneci/toolchain

ENV PATH=/duneci/install/bin:$PATH

RUN echo 'CMAKE_FLAGS+=" -DDUNE_PYTHON_VIRTUALENV_SETUP=1"' >> /duneci/cmake-flags/enable_virtualenv
RUN duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/core/dune-common.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/core/dune-geometry.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/staging/dune-uggrid.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/core/dune-grid.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/core/dune-istl.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/core/dune-localfunctions.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/quality/dune-testtools.git \
    && duneci-install-module -b support/dune-copasi-v0.3 https://gitlab.dune-project.org/copasi/dune-typetree.git \
    && duneci-install-module -b releases/2.7 https://gitlab.dune-project.org/staging/dune-functions.git \
    && duneci-install-module -b support/dune-copasi-v0.3 --recursive https://gitlab.dune-project.org/copasi/dune-logging.git \
    && duneci-install-module -b support/dune-copasi-v0.3 https://gitlab.dune-project.org/copasi/dune-pdelab.git \
    && duneci-install-module -b support/dune-copasi-v0.3 https://gitlab.dune-project.org/copasi/dune-multidomaingrid.git
RUN dunecontrol --opts=/duneci/dune.opts bexec cmake --build . -- install && rm -rf /duneci/modules/*
