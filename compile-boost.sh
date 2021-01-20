#!/bin/bash -e

## module
MODULE=boost
VERSION=1_49_0
BDIR=boost_${VERSION}
BFILE=boost_${VERSION}.tar.gz

## load defaults
. ./common.sh

## check debug switch
#if [ -n "$SW_DEBUG" ]; then
#    CONFIGURE_DEBUG="--enable-symbols"
#fi

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
    if [ -z "$WINDIR" ]; then
        tar -xzf $BASEDIR/backup/$BFILE -C $COMPILEDIR/
    else
        ## mingw throws a checksum error because it's still using 16 bit git_t - forward output to null
        tar -xzf $BASEDIR/backup/$BFILE -C $COMPILEDIR/ 2> /dev/null || true
    fi
    ## create link without version number
    cd $COMPILEDIR
    mv $BDIR $MODULE
    echo done
fi

## Since we're using no precompiled libraries there is no compilation neccessary
## change to compile dir
#cd $COMPILEDIR/${MODULE}_${VERSION}
## prepare for compilation
#./bootstrap.sh -prefix=$COMPILEDIR/${MODULE}-install
## compile
#./b2
## install
#./b2 install

## copy licence file
cp -pv $COMPILEDIR/$MODULE/LICENSE_1_0.txt $iLICENCEDIR/$MODULE.licence

## fini
echo "** Finished compile-$MODULE."
