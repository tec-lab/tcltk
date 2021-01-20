This is the Tcl/Tk compile environment which represents the platform for all gse applications.

Prerequisites:
*) MinGW & MinSYS: Works with mingw-get-inst-20111118.exe downloaded from http://www.mingw.org/
#*) Zip utility for extracting the boost library

Main scripts:
*) common.sh: Stores common configuration.
*) compile_base.sh: Compiles all sub-modules in required order. The following command-line switches are supported:
  .) --debug: Compiles with debug symbols.
  .) --cleanup: Deletes old compile sources and copy from scratch.
  .) --forcecopy: Copy new sources without deleting, thus only new files get compiled.
*) compile_rf-spec.sh: Compile base plus additional specials for RF.
*) compile_pwr-spec.sh: For future.

Directories:
*) backup: Put backups of the external directory here.
*) internal: Repositories maintained or kept in-house.
*) patch: Patch files needed in order to compile the sub-modules.
*) rcompile: Will be created as copy source where compilation is performed.
*) release: Final release directory.

Before adding sub-modules please note that all must be maintened for both windows and linux platforms.
