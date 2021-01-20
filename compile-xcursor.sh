#!/bin/bash -e

## module
MODULE=xcursor

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
    rsync -a $INTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
export INSTALLDIR
if [ -n "$SW_DEBUG" ]; then
    make -f Makefile.debug
else
    make -f Makefile.release
fi

## get version number of module
echo -n "Loading $MODULE package ... "
cd $COMPILEDIR/$MODULE/$OUTDIR
echo "load [file join [pwd] libxcursor$LIBEXT]" > version.tcl
echo "puts [package re XCursor]" >> version.tcl
echo "exit" >> version.tcl
VERSION=$($INSTALLDIR/bin/$WISH version.tcl)
rm ./version.tcl
echo done

## install module
echo -n "Installing ... "
mkdir -p $INSTALLDIR/lib/xcursor$VERSION
cd $INSTALLDIR/lib/xcursor$VERSION
echo "package ifneeded XCursor $VERSION [list load [file join \$dir libxcursor$LIBEXT] xcursor]" > pkgIndex.tcl
cp $COMPILEDIR/$MODULE/$OUTDIR/libxcursor$LIBEXT .
echo done

## copy licence file
echo -n "Copying extras ... "
rsync -a $INTSRCDIR/$MODULE/licence.txt $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
