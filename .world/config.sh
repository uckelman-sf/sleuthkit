#!/bin/bash -ex

. .world/build_config.sh

CONF_FLAGS+=" --disable-java --without-afflib --with-libaff4 --with-libewf --with-libqcow --with-libvhdi --with-libvmdk"

if [ $Target = 'windows' ]; then
  LDFLAGS+=' -fstack-protector'

  if [ $Linkage = 'shared' ]; then
    ln -snf -t unit_tests $(realpath -m $INSTALL/bin/*.dll tsk/.libs/libtsk-19.dll)
    ln -snf -t tests $(realpath -m $INSTALL/bin/*.dll tsk/.libs/libtsk-19.dll)
  fi

  if [ $Linkage = 'static' ]; then
    # for libaff4
    LIBS+="-lshlwapi -lrpcrt4 -luriparser -lraptor2 -luriparser -lpthread -llz4 -lsnappy -lz -lstdc++"
  fi
fi

configure_it
