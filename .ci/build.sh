# build script for all CIs

echo "DUNE_OPTIONS_FILE: ${DUNE_OPTIONS_FILE}"
./dune-common/bin/dunecontrol --opts=${DUNE_OPTIONS_FILE} --module=dune-copasi all