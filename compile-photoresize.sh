#!/bin/bash -e

## module
MODULE=photoresize

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
## patch to MINGW64
sed -i "s|MINGW32_|MINGW64_|g" $COMPILEDIR/$MODULE/configure

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and make
autoconf
./configure --prefix=$INSTALLDIR --enable-64bit
make install

echo -n "Copying extras ... "
## copy licence file
rsync -a $COMPILEDIR/$MODULE/LICENSE $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
