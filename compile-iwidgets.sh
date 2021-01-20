#!/bin/bash -e

## module
MODULE=iwidgets

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
    rsync -a --exclude "*/CVS" $EXTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
./configure --prefix=$INSTALLDIR --with-itcl=$COMPILEDIR/itcl --with-itk=$COMPILEDIR/itk --mandir=$iSHAREDIR
make install

echo -n "Copying extras ... "
## copy licence file
rsync -a $EXTSRCDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
