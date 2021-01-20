#!/bin/bash -e

## module
MODULE=tcllib

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
    rsync -a --exclude "*/.git" $EXTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
./configure --prefix=$INSTALLDIR --exec-prefix=$INSTALLDIR --mandir=$iSHAREDIR
make install
chmod ug+x $INSTALLDIR/bin/*

echo -n "Copying extras ... "
## copy help files
mkdir -p $iSHAREDIR/doc/$MODULE
rsync -a $COMPILEDIR/$MODULE/idoc/www/* $iSHAREDIR/doc/$MODULE/

## copy licence file
rsync -a $COMPILEDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
