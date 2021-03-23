#!/bin/bash -ex

. .world/build_config.sh

install_it

if [ $Target = 'windows' ]; then
  sed -i.bak -e "s/^dlname=''/dlname='..\/bin\/libtsk-19.dll'/ ; s/^library_names=''/library_names='libtsk.dll.a'/ ; s/^old_library=''/old_library='libtsk.a'/" $INSTALL/lib/libtsk.la
fi
