#!/bin/bash -e

## module
MODULE=tclvisa

## load defaults
. ./common.sh

## check debug switch
if [ -n "$SW_DEBUG" ]; then
    CONFIGURE_SWICH+="--enable-symbols"
fi
## check symbol switch
if [ -n "$SW_SYMBOLS" ]; then
    CFLAGS="-g"
fi
## check 64bit platform
if [ "$BARCH" == "x86_64" ]; then
    CONFIGURE_SWITCH+="--enable-64bit "
fi

## print
echo "** Compiling $MODULE $CONFIGURE_SWITCH"

## delete whole compile sources and start from new
if [ -n "$SW_CLEANUP" ]; then
    echo -n "Deleting old sources ..."
    rm -rf $COMPILEDIR/$MODULE
    echo done
fi
## copy source to temp compile path
if [ ! -d $COMPILEDIR/$MODULE ] || [ -n "$SW_FORCECOPY" ]; then
    echo -n "Copying new sources ... "
    rsync -a --exclude "*~" $INTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and build png
autoconf
(CFLAGS="$CFLAGS -I/usr/include/rsvisa" ./configure --prefix=$INSTALLDIR --mandir=$iSHAREDIR $CONFIGURE_SWITCH)
make
make install

echo -n "Copying extras ... "
## copy licence file
rsync -a $INTSRCDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence
## copy the change log
rsync -a $COMPILEDIR/$MODULE/ChangeLog $iCHANGELOGDIR/$MODULE.changelog
echo done

## fini
echo "** Finished compile-$MODULE."
