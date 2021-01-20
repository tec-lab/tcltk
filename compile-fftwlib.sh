#!/bin/bash -e

## module
MODULE=fftwlib
VERSION=3.3.1.pl1
BDIR=fftw-${VERSION}
BFILE=fftw-${VERSION}.tar.gz

## load defaults
. ./common.sh

## check debug switch
if [ -n "$SW_DEBUG" ]; then
    CONFIGURE_DEBUG="--enable-debug"
fi

## print
echo "** Compiling $MODULE"

## delete whole compile sources and start from new
if [ -n "$SW_CLEANUP" ]; then
    echo -n "Deleting old sources ... "
    rm -rf $COMPILEDIR/$BDIR $COMPILEDIR/$MODULE
    echo done
fi
## copy source to temp compile path
if [ ! -d $COMPILEDIR/$MODULE ] || [ -n "$SW_FORCECOPY" ]; then
    echo -n "Copying new sources ... "
    tar -xzf $BASEDIR/backup/$BFILE -C $COMPILEDIR/
    ## create link without version number
    cd $COMPILEDIR
    mv $BDIR $MODULE
    echo done
fi

exit

## change to compile dir
cd $COMPILEDIR/$MODULE
## some files are in wrong format - need to convert before autoconf is running
$DOS2NIX * > /dev/null 2>&1

## configure and make
autoconf
chmod ug+x configure
./configure --with-our-malloc16 --enable-shared --disable-static --enable-threads --with-combined-threads --enable-sse2 $CONFIGURE_DEBUG
make

## copy licence file
cp -pv $COMPILEDIR/$MODULE/COPYRIGHT $iLICENCEDIR/$MODULE.licence

## fini
echo "** Finished compile-$MODULE."
