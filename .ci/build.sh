# build script for all CIs

echo "DUNECONTROL: ${DUNECONTROL}"
echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
cat ${DUNE_OPTIONS_FILE}

${DUNECONTROL} --opts=${DUNE_OPTIONS_FILE} --only=dune-copasi all