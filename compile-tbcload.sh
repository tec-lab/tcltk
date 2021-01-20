#!/bin/bash -e

## module
MODULE=tbcload

## load defaults
. ./common.sh

## check debug switch
if [ -n "$SW_DEBUG" ]; then
    CONFIGURE_SWITCH+="--enable-symbols"
fi

## check 64bit platform
if [ "$BARCH" == "x86_64" ]; then
    CONFIGURE_SWITCH+="--enable-64bit "
fi

## print
echo "** Compiling $MODULE $CONFIGURE_SWITCH"

## delete whole compile sources and start from new
if [ -n "$SW_CLEANUP" ]; then
    echo -n "Deleting old sources ... "
    rm -rf $COMPILEDIR/$MODULE
    echo done
fi
## copy source to temp compile path
if [ ! -d $COMPILEDIR/$MODULE ] || [ -n "$SW_FORCECOPY" ]; then
    echo -n "Copying new sources ... "
    rsync -a $INTSRCDIR/$MODULE $COMPILEDIR/
    rsync -a $EXTSRCDIR/tcl/generic/tclCompile.h $COMPILEDIR/$MODULE/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
#env CFLAGS="-D TCL_86_PLUS -D TCL_862_PLUS"
autoconf
(CFLAGS="-I$COMPILEDIR/tcl/win" ./configure --prefix=$INSTALLDIR -includedir=$COMPILEDIR/tcl/win $CONFIGURE_SWITCH)
make
make install

## copy licence file
echo -n "Copying extras ... "
rsync -a $COMPILEDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence

## Sun, Uni ... etc. copy source files
mkdir -p $iSHAREDIR/source
tar -czf $iSHAREDIR/source/$MODULE.tgz -C $INTSRCDIR/ $MODULE
echo done

## fini
echo "** Finished compile-$MODULE."
