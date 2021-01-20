#!/bin/bash -e

## module
MODULE=bwidget

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

## install
echo -n "Installing ... "
rsync -a $COMPILEDIR/$MODULE $INSTALLDIR/lib/
echo done

## copy licence file
echo -n "Copying extras ... "
rsync -a $EXTSRCDIR/$MODULE/LICENSE.txt $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
