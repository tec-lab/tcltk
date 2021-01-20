#!/bin/bash -e

## module
MODULE=tcl

## load defaults
. ./common.sh

## check debug switch
if [ -n "$SW_DEBUG" ]; then
    CONFIGURE_SWITCH+="--enable-symbols "
fi

## check 64bit platform
if [ "$BARCH" == "x86_64" ]; then
    CONFIGURE_SWITCH+="--enable-64bit "
fi

## print
echo "** Compiling $MODULE $CONFIGURE_SWITCH"

## initialize
createDirs

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
    rsync -a $PATCHDIR/$MODULE/tclsh.ico $COMPILEDIR/$MODULE/win/
    mkdir -p $INSTALLDIR/img
    rsync -a $PATCHDIR/$MODULE/mainicon.png $INSTALLDIR/img/
    rm -rf $COMPILEDIR/$MODULE/.fossil-settings
    echo done
    ## apply mandatory patches
    patch $COMPILEDIR/$MODULE/generic/tclIO.c $PATCHDIR/$MODULE/tcpnodelay.patch
fi

## change to compile dir
if [ -z "$WINDIR" ]; then
    cd $COMPILEDIR/$MODULE/unix
else
    cd $COMPILEDIR/$MODULE/win
fi

## configure and make
autoconf
./configure --prefix=$INSTALLDIR --mandir=$iSHAREDIR --enable-threads $CONFIGURE_SWITCH
# --disable-shared
make
make install

echo -n "Copying extras ... "
## postprocess
cd $INSTALLDIR/bin
if [ -z "$WINDIR" ]; then
    mv tclsh8.6 tclsh86
    rsync -a tclsh86 tclsh
else
    if [ -n "$SW_DEBUG" ]; then
        mv tclsh86g.exe tclsh86.exe
        rsync -a tclsh86.exe tclsh.exe
        rsync -a tcl86g.dll tcl86.dll
    else
        rsync -a tclsh86.exe tclsh.exe
    fi
    rsync -a $PATCHDIR/mingw/x32/*.dll $iSHARELIB32DIR
    rsync -a $PATCHDIR/mingw/x64/*.dll $iSHARELIB64DIR
fi

## install the shared folder
rsync -a $SHAREDIR/* $iSHAREDIR/
## copy licence files
rsync -a $COMPILEDIR/$MODULE/license.terms $iLICENCEDIR/$MODULE.licence
rsync -a $PATCHDIR/mingw/*.licence $iLICENCEDIR
echo done

## fini
echo "** Finished compile-$MODULE."
