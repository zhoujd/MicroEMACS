#!/bin/sh -f
# JASSPA MicroEmacs - www.jasspa.com
# build - JASSPA MicroEmacs build shell script for unix platforms
# Copyright (C) 2001-2009 JASSPA (www.jasspa.com)
# See the file main.c for copying and conditions.
# Copyright (C) 2021 Danny Wilkins (tekk.in)

OPTIONS=
LOGFILE=
LOGFILEA=
MAINTYPE=me
METYPE=
MEDEBUG=
MAKEFILE=
MAKECDEFS=
X11_MAKEINC=/usr/include
X11_MAKELIB=

while [ -n "$1" ]
do
    if [ $1 = "-h" ] ; then
        echo "usage: build [options]"
        echo ""
        echo "Where options can be:-"
        echo "   -C   : Build clean."
        echo "   -D <define>[=<value>]"
        echo "        : Build with given define, (e.g. -D _USETPARM)."
        echo "   -d   : For debug build (output is med)."
        echo "   -h   : For this help page."
        echo "   -l <logfile>"
        echo "        : Set the compile log file"
        echo "   -la <logfile>"
        echo "        : Append the compile log to the given file"
        echo "   -m <makefile>"
        echo "        : Sets the makefile to use where <makefile> can be"
        echo "            aix4.mak, freebsd.mak, freebsd.gmk etc."
        echo "   -ne  : for NanoEmacs build (output is ne)."
        echo "   -p <search-path>"
        echo "        : Sets the default system search path to <search-path>,"
        echo "          default is "'"'"/usr/local/microemacs"'"'
        echo "   -S   : Build clean spotless."
        echo "   -t <type>"
        echo "        : Sets build type:"
        echo "             c  Console support only (Termcap)"
        echo "             w  Window support only (XTerm)"
        echo "             cw Console and window support (default)"
        echo ""
        exit 1
    elif [ $1 = "-C" ] ; then
        OPTIONS=clean
    elif [ $1 = "-D" ] ; then
        shift
        if [ -n "$MAKECDEFS" ] ; then
            MAKECDEFS="$MAKECDEFS -D$1"
        else
            MAKECDEFS="-D$1"
        fi
    elif [ $1 = "-d" ] ; then
        MEDEBUG=d
    elif [ $1 = "-l" ] ; then
        shift
        LOGFILE="$1"
    elif [ $1 = "-la" ] ; then
        shift
        LOGFILEA="$1"
    elif [ $1 = "-m" ] ; then
        shift
        MAKEFILE=$1
    elif [ $1 = "-ne" ] ; then
        MAINTYPE=ne
    elif [ $1 = "-p" ] ; then
        shift
        if [ -n "$MAKECDEFS" ] ; then
            MAKECDEFS="$MAKECDEFS -D_SEARCH_PATH=\\"'"'"$1\\"'"'
        else
            MAKECDEFS="-D_SEARCH_PATH=\\"'"'"$1\\"'"'
        fi
    elif [ $1 = "-S" ] ; then
        OPTIONS=spotless
    elif [ $1 = "-t" ] ; then
        shift
        METYPE=$1
    else
        echo "Error: Unkown option $1, run build -h for help"
        echo ""
        exit 1
    fi
    shift
done

if [ -z "$MAKEFILE" ] ; then
    
    PLATFORM=`uname`
    
    if   [ $PLATFORM = "AIX" ] ; then
        VERSION=`uname -v`
        if [ $VERSION = 5 ] ; then
            MAKEBAS=aix5
        else
            MAKEBAS=aix4
        fi
    elif [ $PLATFORM = "Darwin" ] ; then
        MAKEBAS=darwin
        X11_MAKEINC=/usr/X11R6/include
        X11_MAKELIB=/usr/X11R6/lib
    elif [ $PLATFORM = "FreeBSD" ] ; then
        MAKEBAS=freebsd
        X11_MAKEINC=/usr/X11R6/include
        X11_MAKELIB=/usr/X11R6/lib
    elif [ $PLATFORM = "Linux" ] ; then
        MAKEBAS="linux"
    elif [ $PLATFORM = "OpenBSD" ] ; then
        MAKEBAS=openbsd
        X11_MAKEINC=/usr/X11R6/include
        X11_MAKELIB=/usr/X11R6/lib
    fi

    # use cc by default if available
    if [ -r $MAKEBAS.mak ] ; then
            MAKEFILE=$MAKEBAS.mak
    fi
    if [ -z "$MAKEFILE" ] ; then
        if [ -r $MAKEBAS.gmk ] ; then
                MAKEFILE=$MAKEBAS.gmk
        fi
    fi
    
    if [ `type cc | grep "not found" || type gcc | grep "not found"` ] ; then
        echo "Error: Failed to find a suitable C compiler (cc or gcc), fix your path and/or symlinks"
        exit 1
    fi
    
    if [ -z "$MAKEFILE" ] ; then
        echo "Error: Failed to find a suitable makefile, use the -m option"
        exit 1
    fi
fi

if [ -z "$OPTIONS" ] ; then
    # Check for an X11 install.
    if [ "$METYPE" = "c" ] ; then
        X11_INCLUDE=
    elif [ -z "$X11_INCLUDE" ] ; then
        if [ -r $X11_MAKEINC/X11/Intrinsic.h ] ; then
            X11_INCLUDE=$X11_MAKEINC
        elif [ -r /usr/include/X11/Intrinsic.h ] ; then
            X11_INCLUDE=/usr/include
        else
            echo "No X-11 support found, forcing terminal only."
            X11_INCLUDE=
            METYPE=c
        fi
    fi
    OPTIONS="$MAINTYPE$MEDEBUG$METYPE"
    if [ -n "$X11_INCLUDE" ] ; then
        MAKEWINDEFS=
        MAKEWINLIBS=
        if [ "$X11_INCLUDE" != "/usr/include" ] ; then
            if [ "$X11_INCLUDE" != "$X11_MAKEINC" ] ; then
                MAKEWINDEFS="-I$X11_INCLUDE"
            fi
        fi
        if [ -n "$X11_LIBRARY" ] ; then
            if [ "$X11_LIBRARY" != "$X11_MAKELIB" ] ; then
                MAKEWINLIBS="-L$X11_LIBRARY"
            fi
        fi
        if [ -z "$XPM_INCLUDE" ] ; then
            XPM_INCLUDE="$X11_INCLUDE"
        fi
        if [ -r $XPM_INCLUDE/X11/xpm.h ] ; then
            MAKEWINDEFS="-D_XPM $MAKEWINDEFS"
            if [ ! "$XPM_INCLUDE" = "$X11_INCLUDE" ] ; then
                MAKEWINDEFS="$MAKEWINDEFS -I$XPM_INCLUDE"
            fi
            if [ -n "$XPM_LIBRARY" ] ; then
                if [ ! "$XPM_LIBRARY" = "$X11_MAKELIB" ] ; then
                    MAKEWINLIBS="$MAKEWINLIBS -L$XPM_LIBRARY"
                fi
            fi
            MAKEWINLIBS="$MAKEWINLIBS -lXpm"
        fi
        export MAKEWINDEFS MAKEWINLIBS
    fi
fi
MAKECDEFS="MAKECDEFS=$MAKECDEFS"
if [ -r $MAKEFILE ] ; then
    if [ -n "$LOGFILE" ] ; then
        echo "make -f $MAKEFILE $OPTIONS \"$MAKECDEFS\"" > $LOGFILE 2>&1
        make -f $MAKEFILE $OPTIONS "$MAKECDEFS" > $LOGFILE 2>&1
    else
        if [ -n "$LOGFILEA" ] ; then
            echo "make -f $MAKEFILE $OPTIONS \"$MAKECDEFS\"" >> $LOGFILEA 2>&1
            make -f $MAKEFILE $OPTIONS "$MAKECDEFS" >> $LOGFILEA 2>&1
        else
            echo "make -f $MAKEFILE $OPTIONS \"$MAKECDEFS\""
            make -f $MAKEFILE $OPTIONS "$MAKECDEFS"
        fi
    fi
else
    echo "Error: Failed to find the makefile $MAKEBAS.$MAKEEXT"
fi
