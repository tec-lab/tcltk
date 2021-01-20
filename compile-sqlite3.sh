#!/bin/bash -e

## module
MODULE=sqlite3

## load defaults
. ./common.sh

## check debug switch
if [ -n "$SW_DEBUG" ]; then
    CONFIGURE_SWITCH+="--enable-debug "
fi

## check 64bit platform
if [ "$BARCH" == "x86_64" ]; then
    CONFIGURE_SWITCH+=""
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
    rsync -a --exclude "*.fossil" $EXTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi
## patch to MINGW64
find $COMPILEDIR/$MODULE -name configure | xargs sed -i "s|MINGW32_|MINGW64_|g"

cd $COMPILEDIR/$MODULE
## configure and make
./configure --prefix=$INSTALLDIR --mandir=$iSHAREDIR --enable-fts5 --enable-fts4 --enable-fts3 --enable-json1 --enable-rtree
make tclsqlite3.c
if [ -z "$WINDIR" ]; then
    make libtclsqlite3.la
    cp ./.libs/libtclsqlite3.so ./tclsqlite3.so
else
    sed -i "s|EXTERN int Sqlite3_Init(Tcl_Interp|int Sqlite3_Init(Tcl_Interp|g" $COMPILEDIR/$MODULE/tclsqlite3.c
    if [ -n "$SW_DEBUG" ]; then
        gcc -v -ggdb -DSQLITE_OS_WIN -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RTREE -I$INSTALLDIR/include -L$INSTALLDIR/lib/ -shared tclsqlite3.c -o tclsqlite3.dll -ltcl86g
    else
        gcc -v -DSQLITE_OS_WIN -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS4 -DSQLITE_ENABLE_FTS5 -DSQLITE_ENABLE_JSON1 -DSQLITE_ENABLE_RTREE -I$INSTALLDIR/include -L$INSTALLDIR/lib/ -shared tclsqlite3.c -o tclsqlite3.dll -ltcl86
    fi
fi

## get version number of module
echo -n "Loading $MODULE package ... "
echo "load [file join [pwd] tclsqlite3$LIBEXT]" > version.tcl
echo "puts [package re sqlite3]" >> version.tcl
echo "exit" >> version.tcl
VERSION=$($INSTALLDIR/bin/$TCLSH version.tcl)
rm ./version.tcl
echo done

## install module
echo -n "Installing ... "
mkdir -p $INSTALLDIR/lib/sqlite$VERSION
cd $INSTALLDIR/lib/sqlite$VERSION
echo "package ifneeded sqlite3 $VERSION [list load [file join \$dir tclsqlite3$LIBEXT] sqlite3]" > pkgIndex.tcl
cp $COMPILEDIR/$MODULE/tclsqlite3$LIBEXT .
echo done

## postprocess
echo -n "Copying extras ... "
## copy licence files
rsync -a $COMPILEDIR/$MODULE/autoconf/tea/license.terms $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
