#-------------------------------------------------------------------------------
#
# fftwTest.tcl
#
# some simple sanity checks of tclfftw
#
# $Id: fftwTest.tcl,v 1.4 2010/12/09 12:18:37 schurek.p Exp $
#
# (c)2010 Peter Schurek
#-------------------------------------------------------------------------------

proc complexRandom {reRange imRange} {
    set re [expr {[lindex $reRange 0] + rand() * ([lindex $reRange 1] - [lindex $reRange 0])}]
    set im [expr {[lindex $imRange 0] + rand() * ([lindex $imRange 1] - [lindex $imRange 0])}]

    return [list $re $im]
}

proc complexMagnitude {c} {
    return [expr {sqrt([lindex $c 0] * [lindex $c 0] + [lindex $c 1] * [lindex $c 1])}]
}


set testCase ""

catch {
    #-------------------------------------------------------------------------------
    set testCase "load tclfftw"
    package require tclFFTW
    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::plan"
    set p [fftw::plan 1024 1]
    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::clear"
    fftw::clear $p
    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::getAt"

    foreach i {0 10 511 1023} {
        set cxOut [fftw::getAt $p 0 $i]

        if {[complexMagnitude $cxOut] != 0.} {
            error "at $i, $cxOut != ||0||"
        }
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::get"

    set cxOutList [fftw::get $p 0]

    if {[llength $cxOutList] != 1024} {
        error "||$cxOutList|| != 1024"
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::getFlat"

    set cxOutList [fftw::getFlat $p 0]

    if {[llength $cxOutList] != 2048} {
        error "||$cxOutList|| != 2048"
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::getFlatInt"

    set cxOutList [fftw::getFlatInt $p 0]

    if {[llength $cxOutList] != 2048} {
        error "||$cxOutList|| != 2048"
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::execute"

    fftw::execute $p 0

    foreach i {0 10 511 1023} {
        set cxOut [fftw::getAt $p 0 $i]

        if {[complexMagnitude $cxOut] != 0.} {
            error "at $i, $cxOut != ||0||"
        }
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::scale"

    set scale [fftw::scale $p 0 32767.]

    if {$scale != 1.} {
        error "$scale != 1."
    }

    foreach i {0 10 511 1023} {
        set cxOut [fftw::getAt $p 0 $i]

        if {[complexMagnitude $cxOut] != 0.} {
            error "at $i, $cxOut != ||0||"
        }
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::setAt"

    foreach i {0 10 511 1023} {
        set cxIn [complexRandom {-100. 100.} {-100. 100.}]

        fftw::setAt $p 0 $i $cxIn

        if {$cxIn != [fftw::getAt $p 1 $i]} {
            error "at $i, $cxIn != [fftw::getAt $p 1 $i]"
        }
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "fftw::destroy"

    fftw::destroy $p

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "out-place FFT"

    set p [fftw::plan 1024 0]
    fftw::clear $p

    for {set i 0} {$i < 1024} {incr i} {
        lappend cxInList1 [complexRandom {-100. 100.} {-100. 100.}]
    }

    fftw::set $p 0 0 $cxInList1

    fftw::execute $p 0
    fftw::execute $p 1

    set cxInList2 [fftw::get $p 1]
    set scale [expr {[complexMagnitude [lindex $cxInList2 0]] / [complexMagnitude [lindex $cxInList1 0]]}]

    for {set i 1} {$i < 1024} {incr i} {
        set cxIn1 [lindex $cxInList1 $i]
        set cxIn2 [lindex $cxInList2 $i]

        if {abs([complexMagnitude $cxIn2] / [complexMagnitude $cxIn1] - $scale) > 1.e-6} {
            error "at $i: $cxIn2 / $cxIn1 != $scale"
        }
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------

    package require math::interpolate
    source calcMcWaveform.tcl

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform Newman phases"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 2]
    array set newResult [fftw::calcMcWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 2]

    if {abs($oldResult(-rms) - $newResult(-rms)) > 1.e-12} {
        error "-rms: $oldResult(-rms) <> $newResult(-rms)"
    }

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-12} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    set ix 0
    foreach p_old $oldResult(-pattern) p_new $newResult(-pattern) {
        if {abs($p_old - $p_new) > 1.e-12} {
            puts "old: -pattern\n $oldResult(-pattern)"
            puts "new: -pattern\n $newResult(-pattern)"
            error "-pattern $ix: $p_old <> $p_new"
        }
        incr ix
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform spec phases"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120}]
    array set newResult [fftw::calcMcWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120}]

    if {abs($oldResult(-rms) - $newResult(-rms)) > 1.e-12} {
        error "-rms: $oldResult(-rms) <> $newResult(-rms)"
    }

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-12} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    set ix 0
    foreach p_old $oldResult(-pattern) p_new $newResult(-pattern) {
        if {abs($p_old - $p_new) > 1.e-12} {
            puts "old: -pattern\n $oldResult(-pattern)"
            puts "new: -pattern\n $newResult(-pattern)"
            error "-pattern $ix: $p_old <> $p_new"
        }
        incr ix
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform spec phases, flat"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.}]
    array set newResult [fftw::calcMcWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.}]

    if {abs($oldResult(-rms) - $newResult(-rms)) > 1.e-3} {
        error "-rms: $oldResult(-rms) <> $newResult(-rms)"
    }

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-3} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    set ix 0
    foreach p_old $oldResult(-pattern) p_new $newResult(-pattern) {
        if {abs($p_old - $p_new) > 5.} {
            puts "old: -pattern\n $oldResult(-pattern)"
            puts "new: -pattern\n $newResult(-pattern)"
            error "-pattern $ix: $p_old <> $p_new"
        }
        incr ix
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform spec phases, flat, ssb"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.} \
                             -ssbFreqsList  {0 14180 14190 14200 14300 100000} \
                             -ssbAmpsList   {0.  1.    2.   3.     2.     0.} \
                             -ssbPhaseList  {0. 10.   20.  30.    40.    50.}]
    array set newResult [fftw::calcMcWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.} \
                             -ssbFreqsList  {0 14180 14190 14200 14300 100000} \
                             -ssbAmpsList   {0.  1.    2.   3.     2.     0.} \
                             -ssbPhaseList  {0. 10.   20.  30.    40.    50.}]

    if {abs($oldResult(-rms) - $newResult(-rms)) > 1.e-3} {
        error "-rms: $oldResult(-rms) <> $newResult(-rms)"
    }

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-3} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    set ix 0
    foreach p_old $oldResult(-pattern) p_new $newResult(-pattern) {
        if {abs($p_old - $p_new) > 5.} {
            puts "old: -pattern\n $oldResult(-pattern)"
            puts "new: -pattern\n $newResult(-pattern)"
            error "-pattern $ix: $p_old <> $p_new"
        }
        incr ix
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 4711 -scale no]
    array set newResult [fftw::calcMcWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 4711 -scale no]

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-12} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform spec phases, flat, ssb - side effect"

    unset -nocomplain oldResult
    unset -nocomplain newResult

    set ssbAmpsList       {0.  1.    2.   3.     2.     0.}
    set ssbAmpsList_save  {0.  1.  2.  3.  2.  0.}
    set ssbPhaseList      {0. 10. 20. 30. 40. 50.}
    set ssbPhaseList_save {0. 10. 20. 30. 40. 50.}

    array set oldResult [calcMCWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.} \
                             -ssbFreqsList  {0 14180 14190 14200 14300 100000} \
                             -ssbAmpsList   {0.  1.    2.   3.     2.     0.} \
                             -ssbPhaseList  {0. 10.   20.  30.    40.    50.}]
    array set newResult [fftw::calcMcWaveform \
                             -center 14250 \
                             -startFreqList {14180 14200} \
                             -stopFreqList  {14190 14300} \
                             -spacing 1. \
                             -phaseSeed {100 120} \
                             -flatFreqsList {0 14180 14190 14200 14300 100000} \
                             -flatAmpsList  {0. 1.   2.    3.    2.    0.} \
                             -ssbFreqsList  {0 14180 14190 14200 14300 100000} \
                             -ssbAmpsList   $ssbAmpsList \
                             -ssbPhaseList  $ssbPhaseList]

    foreach amp $ssbAmpsList amp_save $ssbAmpsList_save {
        if {abs($amp - $amp_save) > 1.e-12} {
            error "side effect: $amp <> $amp_save"
        }
    }

    foreach phase $ssbPhaseList phase_save $ssbPhaseList_save {
        if {abs($phase - $phase_save) > 1.e-12} {
            error "side effect: $phase <> $phase_save"
        }
    }

    if {abs($oldResult(-rms) - $newResult(-rms)) > 1.e-3} {
        error "-rms: $oldResult(-rms) <> $newResult(-rms)"
    }

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-3} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    set ix 0
    foreach p_old $oldResult(-pattern) p_new $newResult(-pattern) {
        if {abs($p_old - $p_new) > 5.} {
            puts "old: -pattern\n $oldResult(-pattern)"
            puts "new: -pattern\n $newResult(-pattern)"
            error "-pattern $ix: $p_old <> $p_new"
        }
        incr ix
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    set testCase "calcMcWaveform empty -powerList"

    unset -nocomplain oldResult
    unset -nocomplain newResult
    array set oldResult [calcMCWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 4711 -scale no -powerList {}]
    array set newResult [fftw::calcMcWaveform -center 14250 -startFreqList 14248 -stopFreqList 14252 -spacing 1. -phaseSeed 4711 -scale no -powerList {}]

    if {abs($oldResult(-powerSum) - $newResult(-powerSum)) > 1.e-12} {
        error "-powerSum: $oldResult(-powerSum) <> $newResult(-powerSum)"
    }

    if {[llength $oldResult(-pattern)] != [llength $newResult(-pattern)]} {
        error "-pattern length: $oldResult(-pattern) <> $newResult(-pattern)"
    }

    puts "OK $testCase"

    #-------------------------------------------------------------------------------
    expr 0;
} err

if {$err != 0} {
    puts "FAIL: $testCase $err"
    exit 1
}
exit 0
