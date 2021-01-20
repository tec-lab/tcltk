#!/bin/bash -e
## exit on non-zero status
set -e

## info
VERSION=1.0
MANUFACT="(c) 2019 ATOS IT Solutions and Services GmbH"
AUTHORS="Thomas Perschak"

## defaults
SCRIPTDIR=$(cd $(dirname $0); pwd)
CBOLD="\033[1m"
CEND="\e[0m"

## helper functions
##
function printHelp {
    echo "Install development environment."
    echo "$MANUFACT, $AUTHORS"
    echo "Script version: $VERSION"
    echo ""
    echo "Syntax:"
    echo " devel-setup.sh [<options>]"
    echo "Options (optional):"
    echo " -help: Print this help text."
}
##
## helper functions

## read command line parameters
INDEX=1
while [ $INDEX -le $# ]; do
    case ${!INDEX} in
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

## restart script as sudo user
if [[ $UID -ne 0 ]]; then
    pkexec $(readlink -f $0) "$@"
    #gksudo -S --message "This script needs root privileges to update your system." $0 $@
    # https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=599233
    ## check if we are sudo user
    #if [[ $(groups) =~ "sudo" ]]; then
    #    sudo -p "Provide root password for $(whoami): " bash $0 "$@"
    #else
    #    ## get sudo user
    #    SUUSR=$(getent group sudo | head -1)
    #    SUUSR=${SUUSR##*:}
    #    echo -n "Switching to sudo user "
    #    ssh -X -t -l $SUUSR localhost $0 "$@"
    #fi
    exit $?
fi

## install development tools
apt-get install autoconf libfftw3-dev gcc make libxt-dev libxft-dev libxcursor-dev chrpath libpq5 libfftw3-3 postgresql-server-dev-all
