# unit tests script for all CIs

echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
echo "DUNECONTROL: ${DUNECONTROL}"

${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi make --target build_unit_tests
${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi bexec ctest -j4 -L "unit"