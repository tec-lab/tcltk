#-------------------------------------------------------------------------------
#
# fftwBenchmark.tcl
#
# some simple benchmarks for the FFTW/Tcl library
#
# $Id: fftwBenchmark.tcl,v 1.3 2010/12/02 20:43:11 schurek.p Exp $
#
# (c)2010 Peter Schurek
#-------------------------------------------------------------------------------

package require math::interpolate
package require tcl_fftw 3.2.2
namespace import tcl_fftw::*

source calcMcWaveform.tcl

proc complexRandom {reRange imRange} {
    set re [expr {[lindex $reRange 0] + rand() * ([lindex $reRange 1] - [lindex $reRange 0])}]
    set im [expr {[lindex $imRange 0] + rand() * ([lindex $imRange 1] - [lindex $imRange 0])}]

    return [list $re $im]
}

proc benchmarkInPlace {size} {
    set p [fftw::plan $size 1]
    fftw::clear $p

    set t0 [clock clicks -milli]

    for {set i 0} {$i < $size} {incr i} {
        fftw::setAt $p 0 $i [complexRandom {-100. 100.} {-100. 100.}]
    }

    set t1 [clock clicks -milli]

    for {set i 0} {$i < 10} {incr i} {
        fftw::execute $p 0
    }

    set t2 [clock clicks -milli]

    fftw::destroy $p
    fftw::cleanup
    return "i, $size, [expr {($t2 - $t1) / 1.e4}], [expr {($t1 - $t0) / 1.e3}]"
}


proc benchmarkOutPlace {size} {
    set p [fftw::plan $size 0]
    fftw::clear $p

    set t0 [clock clicks -milli]

    for {set i 0} {$i < $size} {incr i} {
        fftw::setAt $p 0 $i [complexRandom {-100. 100.} {-100. 100.}]
    }

    set t1 [clock clicks -milli]

    for {set i 0} {$i < 10} {incr i} {
        fftw::execute $p 0
    }

    set t2 [clock clicks -milli]

    fftw::destroy $p
    fftw::cleanup
    return "o, $size, [expr {($t2 - $t1) / 1.e4}], [expr {($t1 - $t0) / 1.e3}]"
}


foreach spacing {1. .1 .01 .001 .0001} {
    unset -nocomplain oldResult
    unset -nocomplain newResult

    set t0 [clock clicks -milli]
    for {set i 0} {$i < 10} {incr i} {
        array set oldResult [calcMCWaveform -center 14250 -startFreqList 14100 -stopFreqList 14400 -spacing $spacing -phaseSeed 2]
    }
    set t1 [clock clicks -milli]
    unset -nocomplain oldResult

    set t2 [clock clicks -milli]
    for {set i 0} {$i < 10} {incr i} {
        array set newResult [calcMcWaveform -center 14250 -startFreqList 14100 -stopFreqList 14400 -spacing $spacing -phaseSeed 2]
    }
    set t3 [clock clicks -milli]
    unset -nocomplain newResult

    puts "$spacing, [expr {($t1 - $t0) / 1.e4}], [expr {($t3 - $t2) / 1.e4}]"
}

exit 0

foreach fftSize [lsort -decreasing -integer {1024 10240 102400 1024000 2048000 2820751 2820752 4096000 5948681 5948682 8192000 10240000 20480000 40960000}] {
    puts [benchmarkInPlace $fftSize]
}

foreach fftSize [lsort -decreasing -integer {1024 10240 102400 1024000 2048000 2820751 2820752 4096000 5948681 5948682 8192000 10240000 20480000 40960000}] {
    puts [benchmarkOutPlace $fftSize]
}
