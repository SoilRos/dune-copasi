# system tests script for all CIs

echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
echo "DUNECONTROL: ${DUNECONTROL}"

${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi bexec ctest -j4 -L "DUNE_SYSTEMTEST"