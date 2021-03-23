#!/bin/bash -ex

. .world/build_config.sh

CONF_FLAGS+=" --disable-java --without-afflib --with-libaff4 --with-libewf --with-libqcow --with-libvhdi --with-libvmdk"

if [ $Target = 'windows' ]; then
  if [ $Linkage = 'shared' ]; then
    for i in base img ; do
      ln -snf -t unit_tests/$i $(realpath -m $INSTALL/bin/*.dll tsk/.libs/libtsk-19.dll)
    done
  else
    # we don't have cppunit for 32-bit static Windows
    CONF_FLAGS+=" --disable-cppunit"
  fi
fi

configure_it
