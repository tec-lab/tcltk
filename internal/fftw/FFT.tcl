## TCL Routine for generation of waveformfile for AFQ100B
## uses ifft from tcl math library, only useful for min. 100kHz spacing
## 10000 points ifft takes 1 minute, without fft and 400k carier it takes 200seconds

package require math::fourier 1.0.2
namespace import ::math::fourier::*
package require math::interpolate 1.0.2
namespace import ::math::interpolate::*

load tcl_fftw

set startTime [clock seconds]

## define inputs
set options(-stimulusId) "Noverdrive"
set outputFileId "testdata-fftw"
set options(-minCarrierSpacing) .001
set options(-center) 3625.0
# set options(-signalFrequencies) [list 3605 3615 3617 3619 3624 3626 3632 3634 3636 3645]
# set options(-signalFrequencies) [list 3575 3585 3595 3605 3615]
set options(-signalFrequencies) {}
for {set i -200} {$i < 101} {set i [expr {$i + $options(-minCarrierSpacing)}]} {lappend options(-signalFrequencies) [expr {$options(-center) + $i}]}
set options(-signalGains)  {}
if {$options(-signalGains) == {}} {foreach f $options(-signalFrequencies) {lappend options(-signalGains) 0.0}}
set options(-stimulusSeed)  4711
set options(-gainCables) "FLAT_UL1_VSG1"
set options(-sbsupCable) "IQ_VSG1_SBSUP_RTN"
## only for testing without scoe: data for VSG1 return
set noSCOE true
if {$noSCOE} {
	set options(-gainCables) {}
	set options(-sbsupCable) {}
	set flatFreqs  [list 1626.5 1627.5 1628.5 1629.5 1630.5 1631.5 1632.5 1633.5 1634.5 1635.5 1636.5 1637.5 1638.5 1639.5 1640.5 1641.5 1642.5 1643.5 1644.5 1645.5 1646.5 1647.5 1648.5 1649.5 1650.0 1650.5 1651.5 1652.5 1653.5 1654.5 1655.5 1656.5 1657.5 1658.5 1659.5 1660.5 1668.0 1669.0 1670.0 1671.0 1672.0 1673.0 1674.0 1675.0 3550.0 3552.0 3554.0 3556.0 3558.0 3560.0 3562.0 3564.0 3566.0 3568.0 3570.0 3572.0 3574.0 3576.0 3578.0 3580.0 3582.0 3584.0 3586.0 3588.0 3590.0 3592.0 3594.0 3596.0 3598.0 3600.0 3602.0 3604.0 3606.0 3608.0 3610.0 3612.0 3614.0 3616.0 3618.0 3620.0 3622.0 3624.0 3625.0 3626.0 3628.0 3630.0 3632.0 3634.0 3636.0 3638.0 3640.0 3642.0 3644.0 3646.0 3648.0 3650.0 3652.0 3654.0 3656.0 3658.0 3660.0 3662.0 3664.0 3666.0 3668.0 3670.0 3672.0 3674.0 3676.0 3678.0 3680.0 3682.0 3684.0 3686.0 3688.0 3690.0 3692.0 3694.0 3696.0 3698.0 3700.0]
	set flatAmps   [list 0.014 0.006 0.004 0.002 0 0.002 0.009 0.02 0.035 0.053 0.071 0.091 0.112 0.134 0.158 0.187 0.217 0.238 0.25 0.253 0.249 0.237 0.22 0.189 0.196 0.202 0.249 0.286 0.317 0.346 0.373 0.389 0.397 0.403 0.412 0.427 0.555 0.583 0.614 0.648 0.685 0.724 0.764 0.808 0.357 0.382 0.385 0.404 0.431 0.459 0.467 0.462 0.472 0.487 0.491 0.495 0.495 0.502 0.504 0.494 0.479 0.476 0.475 0.468 0.461 0.453 0.446 0.431 0.401 0.376 0.358 0.334 0.31 0.282 0.265 0.249 0.224 0.199 0.195 0.175 0.131 0.063 0 0.077 0.169 0.25 0.3 0.329 0.377 0.427 0.468 0.516 0.575 0.647 0.713 0.776 0.838 0.909 0.98 1.033 1.086 1.134 1.182 1.227 1.268 1.306 1.349 1.401 1.443 1.479 1.514 1.553 1.575 1.586 1.589 1.594 1.608 1.609 1.597 1.593 1.596]
	set ssbFreqs   [list 1619.99 1624.99 1629.99 1634.99 1639.99 1644.99 1655.01 1660.01 1665.01 1670.01 1675.01 1680.01 1685.01 3544.99 3549.99 3554.99 3559.99 3564.99 3569.99 3574.99 3579.99 3584.99 3589.99 3594.99 3599.99 3604.99 3609.99 3614.99 3619.99 3630.01 3635.01 3640.01 3645.01 3650.01 3655.01 3660.01 3665.01 3670.01 3675.01 3680.01 3685.01 3690.01 3695.01 3700.01 3705.01 3710.01]
	set ssbAmps    [list -0.12 -0.088 -0.062 -0.048 -0.044 -0.04 -0.028 -0.014 -0.002 0.01 0.014 0.022 0.05 -0.184 -0.176 -0.168 -0.156 -0.168 -0.2 -0.238 -0.28 -0.316 -0.34 -0.332 -0.302 -0.272 -0.258 -0.248 -0.242 -0.244 -0.254 -0.254 -0.274 -0.294 -0.328 -0.33 -0.318 -0.274 -0.24 -0.194 -0.172 -0.158 -0.162 -0.17 -0.18 -0.202]
	set ssbPhases  [list -1.42 -1.28 -0.98 -0.66 -0.34 -0.06 0.36 0.54 0.76 1 1.2 1.24 1.2 1.44 1.48 1.52 1.58 1.76 1.86 1.86 1.72 1.5 1.16 0.76 0.54 0.46 0.46 0.46 0.44 0.42 0.32 0.34 0.28 0.24 0 -0.4 -0.74 -0.98 -1.14 -1.1 -1 -0.8 -0.72 -0.72 -0.7 -0.74]
}

## get calibration data from database

if {$options(-gainCables) != {}} {
	set flatFreqs  [[$options(-gainCables) cget -calGainObj]  cget -db_indata ]
	set flatAmps   [[$options(-gainCables) cget -calGainObj]  cget -db_outdata]	
}
if {$options(-sbsupCable) != {}} {
	set ssbFreqs   [[$options(-sbsupCable) cget -calGainObj]  cget -db_indata ]
	set ssbAmps    [[$options(-sbsupCable) cget -calGainObj]  cget -db_outdata]
	set ssbPhases  [[$options(-sbsupCable) cget -calPhaseObj] cget -db_outdata]
}

set pi [expr {4 * atan(1)}]
set clockFrequency 600.0
set fftPoints [expr {round($clockFrequency / $options(-minCarrierSpacing))}]

################ build spectrum list and apply amplitude flatness calibration parameters ######################

## build flatness calibration table for interpolation
set flatTable {}
if {($options(-gainCables) != {}) || $noSCOE} {
		foreach freq $flatFreqs amp $flatAmps {lappend flatTable $freq $amp}
} else {
		## no flatness calibration
		set flatTable {0 0 100000 0} 
}
set signalAmpsCal {}
set carrierIndexList {}
set rmsPower 0
foreach freq $options(-signalFrequencies) amp $options(-signalGains) {
	## calculate rms power by summing the squared linear uncorrected amplitudes
	set rmsPower [expr {$rmsPower + pow(10, $amp / 10.)}]
	## add the interpolated calibration values to the signal amplitudes and de-log
	lappend signalAmpsCal [expr {pow(10,($amp + [::math::interpolate::interp-linear $flatTable $freq]) / 20.)}]
	## assign ifft index to each carrier frequency, build index list
	## carrier indices: index0 = center, index1 = center+minSpacing, indexMax = center-minSpacing
	set index [expr {round(($freq - $options(-center)) / $options(-minCarrierSpacing))}]
	if {$index < 0} {set index [expr {$index + $fftPoints}]}
	lappend carrierIndexList [expr {int($index)}]
}

## assign phases to carriers
set carrierPhases {}
if {[string equal -nocase $options(-stimulusId) "overdrive"]} {
	## apply newman phases if options(-stimulusId) is "overdrive"  p_k = pi*(k-1)^2/N
	set i 0
	foreach idx $carrierIndexList {
		lappend carrierPhases [expr {$pi * $i * $i  / [llength $carrierIndexList]}]
		set i [incr i]
	}
} else {
	## apply random phases
	expr srand($options(-stimulusSeed))
	foreach freq $options(-signalFrequencies) {lappend carrierPhases [expr {rand() * 2. * $pi}]}	
}
## for sorting, build spectrum list with triples: {{index real imag} {index real imag}...}
set spectrum {}
foreach idx $carrierIndexList amp $signalAmpsCal phase $carrierPhases {
	lappend spectrum [list $idx [expr {$amp * cos($phase)}] [expr {$amp * sin($phase)}]]
}	
set spectrum [lsort -integer -index 0 $spectrum]
puts "amplitude calibration = [expr [clock seconds]-$startTime]seconds"; set startTime1 [clock seconds]
################ apply sideband suppression calibration parameters #########################################

if {($options(-sbsupCable) != {}) || $noSCOE} {
	## separate the spectrum in I-spectrum (even) and Q-spectrum (odd) by adding/substracting the mirrored spectrum
	## then apply the ssb calibration data to the Q spectrum only and rebuild the spectrum with spectrum = spectrumI + i*spectrumQ
	## build mirrored spectrumQx, konjugate for I, negative and konjugate for Q (to avoid substraction where order matters)
	set spectrumQx {}
	set spectrumIx {}
	foreach triple $spectrum {
		## idx will become the index for the mirrored frequency
		set idx [expr {round(fmod(($fftPoints - [lindex $triple 0]), $fftPoints))}] 
		lappend spectrumQx [list $idx [expr {-[lindex $triple 1]}] [lindex $triple 2]]
		lappend spectrumIx [list $idx [lindex $triple 1] [expr {-[lindex $triple 2]}]]
	}	
	## spectrumQ = (spectrum + spectrumQx)/2i  spectrumI = (spectrum + spectrumIx)/2
	## concat and sort lists, then add amplitudes with equal frequency indices, which are now consecutive
	set spectrumQx [lsort -integer -index 0 [concat $spectrum $spectrumQx]]
	set spectrumIx [lsort -integer -index 0 [concat $spectrum $spectrumIx]]
	set spectrumQ {}
	set spectrumI {}
	for {set i 0} {$i < [llength $spectrumQx]} {incr i} {
		set idx0 [lindex $spectrumQx $i 0]
		## idx1 is index of next element in list to be checked if equal
		if {$i < ([llength $spectrumQx] - 1)} {set j [expr {$i + 1}]; set idx1 [lindex $spectrumQx $j 0]} else {set idx1 -1}
		if {$idx0 == $idx1} {
			## add both elements with equal index and increment i
			lappend spectrumQ [list $idx0 [expr {([lindex $spectrumQx $i 2] + [lindex $spectrumQx $j 2]) / 2.}]\
										  [expr {([lindex $spectrumQx $i 1] + [lindex $spectrumQx $j 1]) / -2.}]]
			lappend spectrumI [list $idx0 [expr {([lindex $spectrumIx $i 1] + [lindex $spectrumIx $j 1]) / 2.}]\
										  [expr {([lindex $spectrumIx $i 2] + [lindex $spectrumIx $j 2]) / 2.}]]
			set i [incr i]
		} else {
			## only append i-th element
			lappend spectrumQ [list $idx0 [expr {([lindex $spectrumQx $i 2]) / 2.}]\
										  [expr {([lindex $spectrumQx $i 1]) / -2.}]]
			lappend spectrumI [list $idx0 [expr {([lindex $spectrumIx $i 1]) / 2.}]\
										  [expr {([lindex $spectrumIx $i 2]) / 2.}]]
		}
	}
	## amplitude error calibration of spectrumQ for improved sideband suppression
	## create frequency list of spectrumQ indices for interpolation table
	set freqsQ {}
	foreach triple $spectrumQ {
		set idx [lindex $triple 0]
			if {$idx < ($fftPoints / 2)} {
				## TO BE VERIFIED: < OR <= ?
				lappend freqsQ [expr {$options(-center) + $idx * $options(-minCarrierSpacing)}]
			} else {
				lappend freqsQ [expr {$options(-center) + ($idx - $fftPoints) * $options(-minCarrierSpacing)}]
			}
	}
puts "calculate I,Q spectrum = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]
	## create interpolation table for amplitude errors of Q for sideband suppression	
	set ssbAmpTable {}
	foreach freq $ssbFreqs amp $ssbAmps {lappend ssbAmpTable $freq [expr {pow(10, $amp / 20.)}]}
	## apply amplitude error calibration values to real and imaginary part of spectrumQ
	set spectrumQx {}
	foreach triple $spectrumQ freq $freqsQ {
		set ampCorrFactor [::math::interpolate::interp-linear $ssbAmpTable $freq]
		lappend spectrumQx [list [lindex $triple 0]\
								 [expr  {[lindex $triple 1] * $ampCorrFactor}]\
								 [expr  {[lindex $triple 2] * $ampCorrFactor}]]
	}
puts "ssb amplitude calibration = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]
	## phase error calibration of spectrumQ for sideband suppression
	## create interpolation table for phase error correction of Q
	set ssbPhaseTable {}
	set degRad [expr {$pi / 180.}]
	foreach freq $ssbFreqs phase $ssbPhases {lappend ssbPhaseTable $freq [expr {$degRad * $phase}]}
	## apply phase error calibration values by multiplying spectrumQx with exp(-i*phase)
	set spectrumQ {}
	foreach triple $spectrumQx freq $freqsQ {
		set phaseCorrFactor [::math::interpolate::interp-linear $ssbPhaseTable $freq]
		set re [expr {cos($phaseCorrFactor)}]
		set im [expr {sin($phaseCorrFactor)}]
		lappend spectrumQ [list [lindex $triple 0]\
								[expr  {[lindex $triple 1] * $re + [lindex $triple 2] * $im}]\
								[expr  {[lindex $triple 2] * $re - [lindex $triple 1] * $im}]]
	}
puts "ssb phase calibration = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]
	## compose corrected spectrum: spectrum = spectrumI + i*spectrumQ 
	set spectrum {}
	for {set i 0} {$i < [llength $spectrumQ]} {incr i} {
		## only add i-th elements of I and Q
		lappend spectrum [list [lindex $spectrumQ $i 0]\
							   [expr {[lindex $spectrumI $i 1] - [lindex $spectrumQ $i 2]}]\
							   [expr {[lindex $spectrumI $i 2] + [lindex $spectrumQ $i 1]}]]
		}
	unset spectrumI
	unset spectrumQ
	unset spectrumQx
	unset spectrumIx
}
puts "compose spectrum = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

#-setup ------------------------------------------------------------------------
set plan [fftw::plan $fftPoints 1]

fftw::clear $plan

foreach triple $spectrum {
    fftw::setAt $plan 1 [lindex $triple 0] [lrange $triple 1 2]
}

unset spectrum
puts "set spectrum = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

fftw::execute $plan 1
puts "IFFT = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

set timeSamples [eval concat [fftw::get $plan 1]]
puts "IFFT read = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

## scale: search for absolute maximum and scale to +-32767
set max 0
foreach sample $timeSamples {if {abs($sample) > $max} {set max [expr {abs($sample)}]}}

puts "maximum search = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

set scaleFactor [expr {32767.0 / $max}]
set timeSamplesScaled {}
foreach sample $timeSamples {lappend timeSamplesScaled [expr {round($sample * $scaleFactor)}]}
unset timeSamples
## scale the rms value with the same scaling factor
set rmsPower [expr {10 * log10($rmsPower / pow(($max * $fftPoints), 2))}]

puts "scaling = [expr [clock seconds]-$startTime1]seconds"; set startTime1 [clock seconds]

## output to file
set fid [open "${outputFileId}.wv" w]
fconfigure $fid -translation binary
## necessary, otherwise linefeed would be changed to cr lf
puts -nonewline $fid "{TYPE: SMU-WV,0}"
puts -nonewline $fid "{CLOCK: [expr {int($clockFrequency * 1.e6)}]}"
if {$clockFrequency > 301} {puts -nonewline $fid "{CLOCK MARKER:3e+08}"}
puts -nonewline $fid "{COMMENT: ${outputFileId}.wv}"
puts -nonewline $fid "{DATE: [clock format [clock seconds] -format "%Y-%m-%d;%T"]}"
puts -nonewline $fid "{SAMPLES: $fftPoints}"
puts -nonewline $fid "{WAVEFORM-[expr {int(1 + (4 * $fftPoints))}]:#"
puts -nonewline $fid [binary format "s*" $timeSamplesScaled]
puts -nonewline $fid "}"
close $fid
puts "rms value = ${rmsPower} dBm"
puts "overall time = [expr [clock seconds]-$startTime]seconds"
