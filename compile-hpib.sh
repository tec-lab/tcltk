#!/bin/bash -e

## module
MODULE=hpib

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
export INSTALLDIR
make

## get version number of module
echo -n "Loading $MODULE package ... "
echo "load [file join [pwd] libhpib$LIBEXT]" > version.tcl
echo "puts [package re Hpib]" >> version.tcl
echo "exit" >> version.tcl
VERSION=$($INSTALLDIR/bin/$WISH version.tcl)
rm ./version.tcl
echo done

## install module
echo -n "Installing ... "
mkdir -p $INSTALLDIR/lib/hpib$VERSION
cd $INSTALLDIR/lib/hpib$VERSION
echo "package ifneeded Hpib $VERSION [list load [file join \$dir libhpib$LIBEXT] hpib]" > pkgIndex.tcl
cp $COMPILEDIR/$MODULE/libhpib$LIBEXT .
echo done

## fini
echo "** Finished compile-$MODULE."
