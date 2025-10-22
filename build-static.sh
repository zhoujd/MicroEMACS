#!/bin/sh

PLATFORM=`uname`
if [ $PLATFORM = "Linux" ] ; then
    MAKEFILE=linuxgcc-static.mak
else
    echo "Build static on $PLATFORM is not support!"
    exit 1
fi

case $1 in
    build|-b )
        make -f $MAKEFILE
        ;;
    clean|-c )
        make -f $MAKEFILE clean
        ;;
    * )
        echo "Usage: $(basename $0) {build|-b|clean|-c}"
        ;;
esac

