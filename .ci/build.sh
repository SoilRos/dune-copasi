# build script for all CIs

echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
echo "DUNECONTROL: ${DUNECONTROL}"

${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --module=dune-copasi all