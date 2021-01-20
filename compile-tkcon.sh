#!/bin/bash -e

## module
MODULE=tkcon

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

echo -n "Installing ... "
## configure and make
if [ -z "$WINDIR" ]; then
    rsync -a $COMPILEDIR/$MODULE/tkcon.tcl $INSTALLDIR/bin/tkcon
    perl -pi -e "s:exec wish:exec $INSTALLDIR/bin/$WISH:" $INSTALLDIR/bin/tkcon
    chmod 755 $INSTALLDIR/bin/tkcon
else
    rsync -a $COMPILEDIR/$MODULE/tkcon.tcl $INSTALLDIR/bin/
fi
echo done

echo -n "Copying extras ... "
## install the help files
mkdir -p $iSHAREDIR/doc/$MODULE
rsync -a $COMPILEDIR/$MODULE/index.html $iSHAREDIR/doc/$MODULE/
rsync -a $COMPILEDIR/$MODULE/docs/* $iSHAREDIR/doc/$MODULE/
chmod -R a+rX $iSHAREDIR/doc/$MODULE

## copy licence file
rsync -a $EXTSRCDIR/$MODULE/docs/license.terms $iLICENCEDIR/$MODULE.licence
echo done

## fini
echo "** Finished compile-$MODULE."
