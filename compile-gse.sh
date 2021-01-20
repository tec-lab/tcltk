#!/bin/bash -e

## module
MODULE=gse

## load defaults
. ./common.sh "$@" -forcecopy

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
    rsync -a $INTSRCDIR/$MODULE $COMPILEDIR/
    echo done
fi

## get build release
DEBRELEASE=$(awk -F' ' '{if($1=="Release:") print $2}' $SCRIPTDIR/debian.template/control)
if [ -n "$SW_DEBUG" ]; then
    DEBRELEASE=${DEBRELEASE}d
elif [ -n "$SW_SYMBOLS" ]; then
    ## Note: this could also be done using the "objdump --file-headers" command
    DEBRELEASE=${DEBRELEASE}s
fi
BUILDDATE=$(date -u)
BUILDKERNELNAME=$(uname)
BUILDKERNELRELEASE=$(uname -r)
BUILDKERNELVERSION=$(uname -v)
BUILDNODENAME=$(uname -n)
BUILDMACHINE=$(uname -m)
if [ -z "$WINDIR" ]; then 
    BUILDOS=$(lsb_release -sd)
    BUILDOS="${BUILDOS//\"}"
else
    BUILDOS=$(uname -s)
    BUILDOS=${BUILDOS#*_}
fi
GITHASH=$(git log -n 1 --pretty=format:%H)
GCCVERSION=$(gcc --version | head -1 | grep -o "[0-9][.][0-9]*[.][0-9]*[.-]*[0-9]*" | head -1)

## replace the specific variables in gse package
sed -i "s|%tcl_release%|$DEBRELEASE|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_os%|$BUILDOS|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_kernelname%|$BUILDKERNELNAME|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_kernelrelease%|$BUILDKERNELRELEASE|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_kernelversion%|$BUILDKERNELVERSION|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_nodename%|$BUILDNODENAME|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_machine%|$BUILDMACHINE|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%build_date%|$BUILDDATE|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%git_hash%|$GITHASH|g" $COMPILEDIR/$MODULE/pkgIndex.tcl
sed -i "s|%gcc_version%|$GCCVERSION|g" $COMPILEDIR/$MODULE/pkgIndex.tcl

## get gse version number
echo -n "Loading $MODULE package ... "
cd $COMPILEDIR/$MODULE
echo "source pkgIndex.tcl" > ./info.tcl
echo "puts [package re gse]" >> ./info.tcl
echo "exit" >> ./info.tcl
VERSION=$($INSTALLDIR/bin/$TCLSH ./info.tcl)
rm ./info.tcl
echo done

## now copy to release
echo -n "Installing ... "
mkdir -p $INSTALLDIR/lib/$MODULE$VERSION
rsync -a $COMPILEDIR/$MODULE/* $INSTALLDIR/lib/$MODULE$VERSION/
echo done

## fini
echo "** Finished compile-$MODULE"
