#! /bin/bash

################################################################################
# Packages the driving simulator
################################################################################

set -e

DOC_STRING="Packages the driving simulator in Shipping to standalone exe.
Please make sure to run Rebuild.sh before!"

USAGE_STRING="Usage: $0 [-h|--help] [--no-packaging] [--no-zip] [--clean-intermediate]"

# ==============================================================================
# -- Parse arguments -----------------------------------------------------------
# ==============================================================================

DO_PACKAGE=true
DO_CLEAN_INTERMEDIATE=true

OPTS=`getopt -o h --long help,no-packaging,no-zip,clean-intermediate -n 'parse-options' -- "$@"`

if [ $? != 0 ] ; then echo "$USAGE_STRING" ; exit 2 ; fi

eval set -- "$OPTS"

while true; do
  case "$1" in
    --no-packaging )
      DO_PACKAGE=false
      shift ;;
    --clean-intermediate )
      DO_CLEAN_INTERMEDIATE=true
      shift ;;
    -h | --help )
      echo "$DOC_STRING"
      echo "$USAGE_STRING"
      exit 1
      ;;
    * )
      break ;;
  esac
done

# ==============================================================================
# -- Set up environment --------------------------------------------------------
# ==============================================================================

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$SCRIPT_DIR" >/dev/null

REPOSITORY_TAG=`git describe --tags --dirty --always`

echo "Packaging version '$REPOSITORY_TAG'."

UNREAL_PROJECT_FOLDER=${PWD}/Unreal/CarlaUE4
DIST_FOLDER=${PWD}/Dist
BUILD_FOLDER=${DIST_FOLDER}

function fatal_error {
  echo -e "\033[0;31mERROR: $1\033[0m"
  exit 1
}

function log {
  echo -e "\033[0;33m$1\033[0m"
}

#pkill CarlaUE4

# ==============================================================================
# -- Package project -----------------------------------------------------------
# ==============================================================================

if $DO_PACKAGE ; then

  pushd "$UNREAL_PROJECT_FOLDER" >/dev/null

  log "Packaging the project..."

  if [ ! -d "${UE4_ROOT}" ]; then
    fatal_error "UE4_ROOT is not defined, or points to a non-existant directory, please set this environment variable."
  fi

  rm -Rf "${BUILD_FOLDER}/LinuxNoEditor"
#  mkdir -p "${BUILD_FOLDER}"

  "${UE4_ROOT}"/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
      -project="${PWD}/CarlaUE4.uproject" \
      -nocompileeditor -nop4 -cook -stage -archive -package \
      -clientconfig=Shipping -ue4exe=UE4Editor \
      -pak -prereqs \
      -targetplatform=Linux -build -utf8output \
      -archivedirectory=""${BUILD_FOLDER}""

  echo pwd

#  cp -v "./Unreal/CarlaUE4/Config/CarlaSettings.ini" "${BUILD_FOLDER}/LinuxNoEditor/Settings.ini"

  popd >/dev/null

fi

if [[ ! -d "${BUILD_FOLDER}"/LinuxNoEditor ]] ; then
  fatal_error "Failed to package the project!"
fi

mkdir -p "${BUILD_FOLDER}/LinuxNoEditor/CarlaUE4/Config/Experiment Configs"

# ==============================================================================
# -- Zip the project -----------------------------------------------------------
# ==============================================================================

if $DO_TARBALL ; then

  DESTINATION=${DIST_FOLDER}/Driving.zip
  SOURCE="${BUILD_FOLDER}"/LinuxNoEditor

  pushd "$SOURCE" >/dev/null

  log "Packaging build..."

  rm -f ./Manifest_NonUFSFiles_Linux.txt
  rm -Rf ./CarlaUE4/Saved
  rm -Rf ./Engine/Saved

  zip -r ${DESTINATION} *

  popd >/dev/null

fi

# ==============================================================================
# -- Remove intermediate files -------------------------------------------------
# ==============================================================================

if $DO_CLEAN_INTERMEDIATE ; then

  log "Removing intermediate build..."

  rm -Rf "${BUILD_FOLDER}"

fi

echo
echo "Packaged version created at ${FINAL_PACKAGE}"
echo
echo "****************"
echo "*** Success! ***"
echo "****************"

popd >/dev/null
