#!/bin/sh

PLATFORM=`uname`
if [ "$PLATFORM" = "Linux" ]; then
    MAKEFILE=linux.gmk
else
    echo "Build static on $PLATFORM is not support!"
    exit 1
fi

case $1 in
    build|-b )
        make -f $MAKEFILE mec mew
        ;;
    clean|-c )
        make -f $MAKEFILE clean
        ;;
    install|-i )
        if [ "$(whoami)" == "root" ]; then
            make -f $MAKEFILE install
        else
            sudo make -f $MAKEFILE install
        fi
        ;;
    * )
        echo "Usage: $(basename $0) {build|-b|clean|-c|install|-i}"
        ;;
esac
