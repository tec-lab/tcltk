#!/bin/bash -e
## exit on non-zero status
set -e

## info
VERSION=1.3
MANUFACT="(c) 2019 ATOS"
AUTHORS="Thomas Perschak"

## helper functions
##
function printHelp {
    echo "Rebuild and update external repositories."
    echo "$MANUFACT, $AUTHORS"
    echo "Script version: $VERSION"
    echo ""
    echo "Syntax:"
    echo " update-repos.sh [<repo> [<repo> ...]] [<options>]"
    echo "Options (optional):"
    echo " -clean: Cleanup and checkout a fresh repository."
    echo " -help: Print this help text."
    echo ""
    echo "<repo> is a list of repositories to update like tcl, tk, tclconfig, ..."
}

function getbranch {
    ## emty return in case of default branch
    case $1 in
        "tcl"|"tk") echo "core-8-6-branch";;
        "itcl") echo "itcl-4-0-6-rc";;
        "itk") echo "itk-4-0-2-rc";;
        "tkimg") echo "trunk";;
        "thread") echo "thread-2-8-branch";;
    esac
}

function getsource {
    case $1 in
        "tcl") echo "https://core.tcl-lang.org/tcl";;
        "tk") echo "https://core.tcl-lang.org/tk";;
        "thread") echo "https://core.tcl-lang.org/thread";;
        "bwidget") echo "https://core.tcl-lang.org/bwidget";;
        "itcl") echo "https://core.tcl-lang.org/itcl";;
        "itk") echo "https://core.tcl-lang.org/itk";;
        "tclconfig") echo "https://core.tcl-lang.org/tclconfig";;
        "tcllib") echo "https://core.tcl-lang.org/tcllib";;
        "tklib") echo "https://core.tcl-lang.org/tklib";;
        "iwidgets") echo "https://core.tcl-lang.org/iwidgets";;
        "tcludp") echo "https://core.tcl-lang.org/tcludp";;
        "tdbc") echo "https://core.tcl-lang.org/tdbc";;
        "tdbcpostgres") echo "https://core.tcl-lang.org/tdbcpostgres";;
        "tdbcmysql") echo "https://core.tcl-lang.org/tdbcmysql";;
        "tdbcodbc") echo "https://core.tcl-lang.org/tdbcodbc";;
        "tdbcsqlite3") echo "https://core.tcl-lang.org/tdbcsqlite3";;
        "vectcl"|"photoresize") echo "https://github.com/auriocus";;
        "tkcon") echo "https://github.com/wjoye";;
        "tkimg") echo "https://svn.code.sf.net/p/tkimg/code";;
        "sqlite3") echo "http://www.sqlite.org/cgi/src";;
        "tktable") echo ":pserver:anonymous@tktable.cvs.sourceforge.net:/cvsroot/tktable";;
    esac
}

function getstype {
    case $1 in
        "https://github.com/"*) echo "git";;
        ":pserver:anonymous"*) echo "cvs";;
        *"svn.sourceforge.net/svnroot/"*) echo "svn";;
        *"svn.code.sf.net"*) echo "svn";;
        "http://www.sqlite.org/cgi/src"|"https://core.tcl-lang.org"*) echo "fossil";;
    esac
}
##
## helper functions

## defaults
SCRIPTDIR=$(cd $(dirname $0); pwd)

## check command line parameters
INDEX=1
MODULES=()
while [ $INDEX -le $# ]; do
    if [ "${!INDEX:0:1}" != "-" ]; then
        MODULES+=(${!INDEX})
        let INDEX+=1
        continue
    fi
    case ${!INDEX} in
        "-clean")
            CLEAN=1
            ;;
        "-help")
            printHelp
            exit 1
            ;;
        *)
            echo "Unknown parameter '${!INDEX}', type -help for more information." >&2; exit 1
            ;;
    esac
    let INDEX+=1
done

## if no module provided use default
if [ ${#MODULES[@]} -eq 0 ]; then
    MODULES=(tclconfig tcl thread tk tcllib tklib tkimg tdbc tdbcodbc tdbcpostgres tdbcmysql tdbcsqlite3 sqlite3 bwidget vectcl photoresize tkcon tcludp itcl itk iwidgets tktable)
fi

## go through all repos
for MODULE in ${MODULES[@]}; do
    SOURCE=$(getsource $MODULE)
    if [ -z "$SOURCE" ]; then echo "Undefined source for '$MODULE' module." >&2; exit 1; fi
    STYPE=$(getstype $SOURCE)
    if [ -z "$STYPE" ]; then echo "Undefined source type for '$MODULE' module." >&2; exit 1; fi
    BRANCH=$(getbranch $MODULE)
    REPO=$SOURCE/$MODULE.git
    if [ -n "$CLEAN" ] || [ ! -e $SCRIPTDIR/$MODULE ]; then
        if [ -n "$BRANCH" ]; then
            echo "** Rebuilding $MODULE from $SOURCE on branch $BRANCH"
        else
            echo "** Rebuilding $MODULE from $SOURCE"
        fi
    else
        echo "** Refreshing $MODULE"
    fi
    case $STYPE in
        "git")
            if [ -n "$CLEAN" ] || [ ! -e $SCRIPTDIR/$MODULE ]; then
                rm -rf $SCRIPTDIR/$MODULE
                if [ -n "$BRANCH" ]; then
                    cd $SCRIPTDIR
                    git clone $REPO --branch $BRANCH
                else
                    cd $SCRIPTDIR
                    git clone $REPO
                fi
            else
                cd $SCRIPTDIR/$MODULE
                git pull
            fi
            ;;
        "cvs")
            if [ -n "$CLEAN" ] || [ ! -e $SCRIPTDIR/$MODULE ]; then
                rm -rf $SCRIPTDIR/$MODULE
                if [ -n "$BRANCH" ]; then
                    cd $SCRIPTDIR
                    cvs -d $SOURCE checkout -r $BRANCH $MODULE
                else
                    cd $SCRIPTDIR
                    cvs -d $SOURCE checkout $MODULE
                fi
            else
                cd $SCRIPTDIR/$MODULE
                cvs -d $SOURCE update -d
            fi
            ;;
        "svn")
            if [ -n "$CLEAN" ] || [ ! -e $SCRIPTDIR/$MODULE ]; then
                rm -rf $SCRIPTDIR/$MODULE
                if [ -n "$BRANCH" ]; then
                    cd $SCRIPTDIR
                    svn checkout $SOURCE/$BRANCH $MODULE
                else
                    echo "SVN module '$MODULE' must provide a branch." >&2; exit 1
                fi
            else
                cd $SCRIPTDIR/$MODULE
                svn cleanup
                svn update
            fi
            ;;
        "fossil")
            if [ -n "$CLEAN" ] || [ ! -e $SCRIPTDIR/$MODULE ]; then
                rm -rf $SCRIPTDIR/$MODULE
                mkdir -p $SCRIPTDIR/$MODULE
                cd $SCRIPTDIR/$MODULE
                fossil clone $SOURCE ${MODULE}.fossil
                fossil open ${MODULE}.fossil
                if [ -n "$BRANCH" ]; then
                    fossil update $BRANCH
                else
                    fossil update trunk
                fi
            else
                cd $SCRIPTDIR/$MODULE
                fossil pull $SOURCE
                if [ -n "$BRANCH" ]; then
                    fossil update $BRANCH
                else
                    fossil update trunk
                fi
            fi
            ;;
        default)
            echo "Unknown source type '$STYPE' for '$MODULE' module." >&2; exit 1
            ;;
    esac
    echo ""
done

## fini
echo "** Finished update-repos."
