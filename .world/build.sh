#!/bin/bash -ex

. .world/build_config.sh

make_it

if [ "$Target" = 'windows' ]; then
  MAKE_FLAGS+=" LOG_COMPILER=$(realpath .world/wine_wrapper.sh)"
fi

make_check_it
