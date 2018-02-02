#!/bin/bash

#  Copyright 2015 The WebRTC project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.

# Generates static or dynamic FAT libraries for ios in out_ios_libs.

# Exit on errors.
set -e

# Globals.
SCRIPT_DIR=$(cd $(dirname $0) && pwd)
WEBRTC_BASE_DIR=${SCRIPT_DIR}/../../..
GYP_WEBRTC_SCRIPT=${WEBRTC_BASE_DIR}/webrtc/build/gyp_webrtc.py
MERGE_SCRIPT=${SCRIPT_DIR}/merge_ios_libs.py
LICENSE_SCRIPT=${SCRIPT_DIR}/generate_licenses.py

function check_preconditions {
  # Check for Darwin.
  if [[ ! $(uname) = "Darwin" ]]; then
    echo "OS/X required." >&2
    exit 1
  fi

  # Check for libtool.
  if [[ -z $(which libtool) ]]; then
    echo "Missing libtool binary." >&2
    exit 1
  fi

  # Check for GYP generator.
  if [[ ! -x ${GYP_WEBRTC_SCRIPT} ]]; then
    echo "Failed to find gyp generator." >&2
    exit 1
  fi

  # Check for merge script.
  if [[ ! -x ${MERGE_SCRIPT} ]]; then
    echo "Failed to find library merging script." >&2
    exit 1
  fi
}

function build_webrtc {
  local base_output_dir=$1
  local flavor=$2
  local target_arch=$3
  local build_type=$4

  local ninja_output_dir=${base_output_dir}/${target_arch}_ninja
  local library_output_dir=${base_output_dir}/${target_arch}_libs
  if [[ ${target_arch} = 'arm' || ${target_arch} = 'arm64' ]]; then
    flavor="${flavor}-iphoneos"
  else
    flavor="${flavor}-iphonesimulator"
  fi
  local ninja_flavor_dir=${ninja_output_dir}/${flavor}

  # Compile framework by default.
  local gyp_file=webrtc/sdk/sdk.gyp
  local gyp_target=rtc_sdk_framework_objc
  # Set to 1 to explicitly not hide symbols. We'll want this if we're just
  # generating static libs.
  local override_visibility=0
  if [[ ${build_type} = "legacy" ]]; then
    echo "Building legacy."
    gyp_file=webrtc/build/ios/merge_ios_libs.gyp
    gyp_target=libjingle_peerconnection_objc_no_op
    override_visibility=1
  elif [[ ${build_type} = "static_only" ]]; then
    echo "Building static only."
    gyp_file=webrtc/build/ios/merge_ios_libs.gyp
    gyp_target=rtc_sdk_peerconnection_objc_no_op
    override_visibility=1
  elif [[ ${build_type} == "framework" ]]; then
    echo "Building framework."
  else
    echo "Unexpected build type: ${build_type}"
    exit 1
  fi

  export GYP_DEFINES="OS=ios target_arch=${target_arch} use_objc_h264=1 \
clang_xcode=1 ios_deployment_target=8.0 \
ios_override_visibility=${override_visibility}"
  export GYP_GENERATORS="ninja"
  export GYP_GENERATOR_FLAGS="output_dir=${ninja_output_dir}"

  # GYP generation requires relative path for some reason.
  pushd ${WEBRTC_BASE_DIR}
  webrtc/build/gyp_webrtc.py ${gyp_file}
  popd
  # Compile the target we're interested in.
  ninja -C ${ninja_flavor_dir} ${gyp_target}

  if [[ ${build_type} = "framework" ]]; then
    # Manually generate the dSYM files before stripping them. GYP does not seem
    # to instruct ninja to generate dSYM files.
    dsymutil --out=${ninja_flavor_dir}/WebRTC.framework.dSYM \
        ${ninja_flavor_dir}/WebRTC.framework/WebRTC
  fi

  # Make links to the generated static archives.
  mkdir -p ${library_output_dir}
  for f in ${ninja_flavor_dir}/*.a
  do
    ln -sf "${f}" "${library_output_dir}/$(basename ${f})"
  done
}

function clean_artifacts {
  local output_dir=$1
  if [[ -d ${output_dir} ]]; then
    rm -r ${output_dir}
  fi
}

function usage {
  echo "WebRTC iOS FAT libraries build script."
  echo "Each architecture is compiled separately before being merged together."
  echo "By default, the fat libraries will be created in out_ios_libs/fat_libs."
  echo "The headers will be copied to out_ios_libs/include."
  echo "Usage: $0 [-h] [-b build_type] [-c] [-o output_dir]"
  echo "  -h Print this help."
  echo "  -b The build type. Can be framework, static_only or legacy."
  echo "     Defaults to framework."
  echo "  -c Removes generated build output."
  echo "  -o Specifies a directory to output build artifacts to."
  echo "     If specified together with -c, deletes the dir."
  echo "  -r Specifies a revision number to embed if building the framework."
  exit 0
}

check_preconditions

# Set default arguments.
# Output directory for build artifacts.
OUTPUT_DIR=${WEBRTC_BASE_DIR}/out_ios_libs
# The type of build to perform. Valid arguments are framework, static_only and
# legacy.
BUILD_TYPE="framework"
PERFORM_CLEAN=0
FLAVOR="Profile"
POINT_VERSION="0"

# Parse arguments.
while getopts "hb:co:r:" opt; do
  case "${opt}" in
    h) usage;;
    b) BUILD_TYPE="${OPTARG}";;
    c) PERFORM_CLEAN=1;;
    o) OUTPUT_DIR="${OPTARG}";;
    r) POINT_VERSION="${OPTARG}";;
    *)
      usage
      exit 1
      ;;
  esac
done

if [[ ${PERFORM_CLEAN} -ne 0 ]]; then
  clean_artifacts ${OUTPUT_DIR}
  exit 0
fi

# Build all the common architectures.
ARCHS=( "arm" "arm64" "ia32" "x64" )
for ARCH in "${ARCHS[@]}"
do
  echo "Building WebRTC arch: ${ARCH}"
  build_webrtc ${OUTPUT_DIR} ${FLAVOR} $ARCH ${BUILD_TYPE}
done

ARM_NINJA_DIR=${OUTPUT_DIR}/arm_ninja/${FLAVOR}-iphoneos
ARM64_NINJA_DIR=${OUTPUT_DIR}/arm64_ninja/${FLAVOR}-iphoneos
IA32_NINJA_DIR=${OUTPUT_DIR}/ia32_ninja/${FLAVOR}-iphonesimulator
X64_NINJA_DIR=${OUTPUT_DIR}/x64_ninja/${FLAVOR}-iphonesimulator

if [[ ${BUILD_TYPE} = "framework" ]]; then
  # Merge the framework slices together into a FAT library by copying one arch
  # output and merging the rest in.
  DYLIB_PATH="WebRTC.framework/WebRTC"
  cp -R ${ARM_NINJA_DIR}/WebRTC.framework ${OUTPUT_DIR}
  rm ${OUTPUT_DIR}/${DYLIB_PATH}
  echo "Merging framework slices."
  lipo ${ARM_NINJA_DIR}/${DYLIB_PATH} \
      ${ARM64_NINJA_DIR}/${DYLIB_PATH} \
      ${IA32_NINJA_DIR}/${DYLIB_PATH} \
      ${X64_NINJA_DIR}/${DYLIB_PATH} \
      -create -output ${OUTPUT_DIR}/${DYLIB_PATH}

  # Merge the dSYM files together in a similar fashion.
  DSYM_PATH="WebRTC.framework.dSYM/Contents/Resources/DWARF/WebRTC"
  cp -R ${ARM_NINJA_DIR}/WebRTC.framework.dSYM ${OUTPUT_DIR}
  rm ${OUTPUT_DIR}/${DSYM_PATH}
  echo "Merging dSYM slices."
  lipo ${ARM_NINJA_DIR}/${DSYM_PATH} \
      ${ARM64_NINJA_DIR}/${DSYM_PATH} \
      ${IA32_NINJA_DIR}/${DSYM_PATH} \
      ${X64_NINJA_DIR}/${DSYM_PATH} \
      -create -output ${OUTPUT_DIR}/${DSYM_PATH}

  # Strip the dynamic framework of non-global symbols.
  # TODO(tkchin): Override chromium strip settings in supplement.gypi instead.
  echo "Stripping non-global symbols."
  strip -x ${OUTPUT_DIR}/${DYLIB_PATH}

  # Modify the version number.
  INFOPLIST_PATH=${OUTPUT_DIR}/WebRTC.framework/Info.plist
  MAJOR_MINOR=$(plistbuddy -c "Print :CFBundleShortVersionString" \
                ${INFOPLIST_PATH})
  VERSION_NUMBER="${MAJOR_MINOR}.${POINT_VERSION}"
  echo "Substituting revision number: ${VERSION_NUMBER}"
  plistbuddy -c "Set :CFBundleVersion ${VERSION_NUMBER}" ${INFOPLIST_PATH}
  plutil -convert binary1 ${INFOPLIST_PATH}

  # Copy pod file.
  FORMAT_STRING=s/\${FRAMEWORK_VERSION_NUMBER}/${VERSION_NUMBER}/g
  sed -e ${FORMAT_STRING} ${WEBRTC_BASE_DIR}/webrtc/sdk/objc/WebRTC.podspec > \
      ${OUTPUT_DIR}/WebRTC.podspec
else
  echo "Merging static library slices."
  # Merge the static libraries together into individual FAT archives.
  ${MERGE_SCRIPT} ${OUTPUT_DIR}

  # Merge the dSYM files together.
  TARGET_NAME="rtc_sdk_peerconnection_objc_no_op"
  if [[ ${BUILD_TYPE} = "legacy" ]]; then
    TARGET_NAME="libjingle_peerconnection_objc_no_op"
  fi
  DSYM_PATH="${TARGET_NAME}.app.dSYM/Contents/Resources/DWARF/${TARGET_NAME}"
  cp -R ${ARM_NINJA_DIR}/${TARGET_NAME}.app.dSYM ${OUTPUT_DIR}
  echo "Merging dSYM slices."
  lipo ${ARM_NINJA_DIR}/${DSYM_PATH} \
      ${ARM64_NINJA_DIR}/${DSYM_PATH} \
      ${IA32_NINJA_DIR}/${DSYM_PATH} \
      ${X64_NINJA_DIR}/${DSYM_PATH} \
      -create -output ${OUTPUT_DIR}/${DSYM_PATH}

  # Strip debugging symbols.
  # TODO(tkchin): Override chromium settings in supplement.gypi instead to do
  # stripping at build time.
  echo "Stripping debug symbols."
  strip -S ${OUTPUT_DIR}/fat_libs/*.a

  # Symlink the headers.
  echo "Symlinking headers."
  INPUT_HEADER_DIR="${WEBRTC_BASE_DIR}/webrtc/sdk/objc/Framework/Headers/WebRTC"
  OUTPUT_HEADER_DIR="${OUTPUT_DIR}/include"
  if [[ -d ${OUTPUT_HEADER_DIR} ]]; then
    rm -rf ${OUTPUT_HEADER_DIR}
  fi
  if [[ ${BUILD_TYPE} = "legacy" ]]; then
    INPUT_HEADER_DIR="${WEBRTC_BASE_DIR}/talk/app/webrtc/objc/public"
    ln -sf ${INPUT_HEADER_DIR} ${OUTPUT_HEADER_DIR}
  else
    mkdir -p ${OUTPUT_HEADER_DIR}
    ln -sf ${INPUT_HEADER_DIR} ${OUTPUT_HEADER_DIR}/WebRTC
  fi
fi

echo "Generating LICENSE.html."
${LICENSE_SCRIPT} ${OUTPUT_DIR}/arm64_libs ${OUTPUT_DIR}

echo "Done!"
