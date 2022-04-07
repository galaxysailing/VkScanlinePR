#!/bin/bash

SCRIPT_DIR=$(dirname $(readlink -f "$0"))
MAIN_DIR=$SCRIPT_DIR/..

if [ -z $BUILD_DIR ]; then
  BUILD_DIR=$MAIN_DIR/build
fi

function usage() {
  local help="Usage: $0 <deploy>"

  echo "$help"
}

function env_deploy() {
  CMAKE_OPTIONS="${CMAKE_OPTIONS} -DFORCE_BUILD_TYPE=RelWithDebInfo"
}

function build() {
  if [ ! -d $BUILD_DIR ]; then
    mkdir -p $BUILD_DIR
  fi

  cd $BUILD_DIR
  cmake $MAIN_DIR $CMAKE_OPTIONS && cmake --build . $*
}

## Start from here. #####################################
input=$1

case "$input" in
"-h" | "--help")
  usage
  exit 1
  ;;

"deploy")
  env_deploy

  build ${@:2}
  ;;
*)
  build ${@:1}
  ;;
esac
