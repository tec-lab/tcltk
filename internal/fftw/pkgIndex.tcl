#------------------------------------------------------------------------------
#
# pkgIndex.tcl
#
# The Tcl interpreter executes this file for the first time it encounters a
#
#     package require tcl_fftw 3.2.2
#
# command. It loads the shared FFTW library andtogether with the C glue and
# makes the implemented commands accessible
#
# $Id: pkgIndex.tcl,v 1.4 2010/12/02 14:17:07 schurek.p Exp $
#
# (c)2010 Peter Schurek
#------------------------------------------------------------------------------

namespace eval ::tcl_fftw {
    proc load_with_path {dir} {
        set ext [info sharedlibextension];      # learn .dll or .so

        if {[string equal -nocase ".DLL" $ext]} {
            set tempPath $::env(PATH);          # save orignal PATH
            append ::env(PATH) ";$dir";         # append directory of this pkgIndex.tcl file
            load "tcl_fftw${ext}";              # load glue and FFTW library, provide package
            set ::env(PATH) $tempPath;          # restore PATH
        } else {
            load [file join $dir "tcl_fftw${ext}"];     # load glue, provide package
        }

        # export defined functions
        namespace export fft_* calcMcWaveform
    }
}

package ifneeded tcl_fftw 3.2.2 [list ::tcl_fftw::load_with_path $dir]
