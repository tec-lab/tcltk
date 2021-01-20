#!/bin/bash -e

## module
MODULE=vxigate

## load defaults
. ./common.sh

## print
echo "** Compiling $MODULE"

## delete whole compile sources and start from new
if [ -n "$SW_CLEANUP" ]; then
    echo -n "Deleting old sources ... "
    rm -rf $COMPILEDIR/$MODULE
    echo done
fi
## copy source to temp compile path
if [ ! -d $COMPILEDIR/$MODULE ] || [ -n "$SW_FORCECOPY" ]; then
    echo -n "Copying new sources ... "
    rsync -a --exclude "*/.git" $INTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
make
## install
rsync -a $COMPILEDIR/$MODULE/libsicl.so $INSTALLDIR/lib/
rsync -a $COMPILEDIR/$MODULE/sicl.h $INSTALLDIR/include/
rsync -a $COMPILEDIR/$MODULE/vxi11core.h $INSTALLDIR/include/

## fini
echo "** Finished compile-$MODULE."
