#!/bin/bash -e

## module
MODULE=tkimg

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
    rsync -a --exclude "*/.svn" $EXTSRCDIR/$MODULE $COMPILEDIR/
    #if [ -n "$WINDIR" ]; then find $COMPILEDIR/$MODULE | xargs dos2unix; fi
    echo done
fi
## patch to MINGW64
#find $COMPILEDIR/$MODULE -name configure | xargs sed -i "s|MINGW32_[*]|MINGW64_*|g"
#find $COMPILEDIR/$MODULE -name tcl.m4 | xargs sed -i "s|MINGW32_\*|MINGW64_\*|g"

## configure jpeg compat module which creates the needed jconfig.h
cd $COMPILEDIR/$MODULE/compat/libjpeg
./configure

## change to compile dir
cd $COMPILEDIR/$MODULE

## configure and build png
(CFLAGS="$CFLAGS" ./configure --prefix=$INSTALLDIR --mandir=$iSHAREDIR $CONFIGURE_SWITCH)
make
make install

## bug patch for https://sourceforge.net/p/tkimg/bugs/83/
#sed -i "s|img::tiff 1.4.4|img::tiff 3.9.7|" $INSTALLDIR/lib/Img1.4.4/pkgIndex.tcl

echo -n "Copying extras ... "
## copy licence file
rsync -a $EXTSRCDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence
## copy the change log
rsync -a $COMPILEDIR/$MODULE/ChangeLog $iCHANGELOGDIR/$MODULE.changelog
echo done

## fini
echo "** Finished compile-$MODULE."
