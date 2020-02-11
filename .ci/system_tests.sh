# system tests script for all CIs

# make sure we get the right mingw64 version of g++ on appveyor
PATH=/mingw64/bin:$PATH
echo "PATH=$PATH"
echo "MSYSTEM: $MSYSTEM"
echo "DUNECONTROL: ${DUNECONTROL}"
echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
cat ${DUNE_OPTIONS_FILE}
echo "PWD: $PWD"
tree || la

which g++
which python
which cmake
g++ --version
gcc --version
cmake --version

${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi make --target build_system_tests
${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi bexec ctest -j4 -L "DUNE_SYSTEMTEST" --output-on-failure