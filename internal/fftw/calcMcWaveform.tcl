#### calcMCWaveform #######################################################################################

## calculate equalized multi carrier waveform
## example for testing:

proc valbool {v} { return [string match -nocase "y" $v] }

proc calcMCWaveform {args} {

    #%ProcedureRange
    set optionsRange(-center)     {-minmax 0:+}
    set optionsRange(-spacing)    {-minmax {0.0001:600}}
    set optionsRange(-sampleRate) {-minmax {0.001:600}}
    set optionsRange(-scale)      {-values {y n yes no t f true false 1 0} -case no}

    #%ProcedureArgs
    set options(-center)         {14250}
    set options(-phaseSeed)      {2}
    set options(-spacing)        {1.}
    set options(-sampleRate)     {600}
    set options(-scale)          {y}
    set options(-startFreqList)      {}
    set options(-stopFreqList)       {}
    set options(-powerList)          {0.}
    set options(-flatFreqsList)      {0. 100000.}
    set options(-flatAmpsList)       {0. 0.}
    set options(-ssbFreqsList)       {0. 100000.}
    set options(-ssbAmpsList)        {0. 0.}
    set options(-ssbPhaseList)       {0. 0.}

    #%EndArgs
    set allowed [array names options]
    array set options $args
    ## check for unallowed/range options
#    checkCompAllowed options $allowed
#    checkCompRange options optionsRange

    set fftPoints [expr {round($options(-sampleRate) / $options(-spacing))}]
    set fftPoints2 [expr {$fftPoints / 2}]
    set plan [fftw::plan $fftPoints 1]
    fftw::clear $plan

#    Log d "FFTW plan allocated"

    ## check equal list lengths
    if {([llength $options(-powerList)] > 1) && ([llength $options(-startFreqList)] != [llength $options(-powerList)])} {
        error "calcMCWaveform: startFreqList and powerList must have equal lengths"
    }
    if {[llength $options(-powerList)] < 2} {
        ## assume equal amplitudes for all carriers if ampList contains less than 2 elements
        if {[llength $options(-powerList)] == 0} {set options(-powerList) 0}
        set options(-powerList) [lrepeat [llength $options(-startFreqList)] [lindex $options(-powerList) 0]]
    }

#    set ifCenter [CONF getval intermCenter]
    set pi [expr {4*atan(1)}]
    set startTime [clock clicks -milli]

    ## BEGIN cripple finally in order to deallocate FFTW plan
    if {[catch {

        ## calculate frequency list: if stopFreqList is empty, copy it from startFreqList => single carriers are generated
        if {$options(-stopFreqList) == {}} {set options(-stopFreqList) $options(-startFreqList)}
        ## calculate with integer numbers, frequencies in 100Hz:
        set center [expr {round($options(-center) * 10000)}]
        set spacing [expr {round($options(-spacing) * 10000)}]
        ## spectrumRe is at the moment not the real part, but the uncalibrated magnitude of the spectrum
        set spectrumRe [lrepeat $fftPoints 0.]
        set freqCount 0
        foreach fstart $options(-startFreqList) fstop $options(-stopFreqList) pwr $options(-powerList) {
            set f1 [expr {round($fstart * 10000)}]
            set f2 [expr {round($fstop * 10000)}]
            ## calculate amplitude per carrier in Volts
            set apc [expr {pow(10,$pwr / 20.) / sqrt(1. + ($f2 - $f1) / $spacing)}]
            set freqOld $fstart

            for {set freq $f1} {$freq <= $f2} {set freq [expr {$freq + $spacing}]} {
                ## calculate fft index : index0 = center, index1 = center+spacing, indexMax = center-Spacing
                set idx [expr {round(($freq - $center) / $spacing)}]
                if {$idx < 0} {set idx [expr {$idx + $fftPoints}]}
                lset spectrumRe $idx $apc
                incr freqCount
            }
        }


        ## spectrumIm is at the moment not the imaginary part, but the phase angle
        set spectrumIm [lrepeat $fftPoints 0.]
        # assign phases to carriers
        if {[llength $options(-phaseSeed)] > 1} {
            if {([llength $options(-phaseSeed)] != [llength $options(-startFreqList)]) && ($options(-startFreqList) != $options(-stopFreqList))} {
                error: "calcMCWaveform: phaseSeed contains more than one value and list length [llength $options(-phaseSeed)] \
                                is different from startFreqList [llength $options(-startFreqList)] or stopFreqList is different from startFreqList!"
            }
            foreach freq $options(-startFreqList) phase $options(-phaseSeed) {
                set idx [expr {round((10000. * $freq - $center) / $spacing)}]
                if {$idx < 0} {set idx [expr {$idx + $fftPoints}]}
                lset spectrumIm $idx $phase
            }
        } else {
            if {$options(-phaseSeed) < 11} {
                ## apply newman phases if options(-phaseSeed) = 2  p_k = pi*(k-1)^2/N
                set j 0
                for {set i $fftPoints2} {$i < [expr {$fftPoints2 + $fftPoints}]} {incr i} {
                    if {$i >= $fftPoints} {set k [expr {$i - $fftPoints}]} else {set k $i}
                    if {[lindex $spectrumRe $k] == 0} {continue}
                    lset spectrumIm $k [expr {$pi * pow($j, $options(-phaseSeed)) / $freqCount}]
                    incr j
                }
            } else {
                ## apply random phases
                expr srand(round($options(-phaseSeed)))
                set pi2 [expr {$pi * 2.0}]
                for {set i 0} {$i < [llength $spectrumRe]} {incr i} {
                    if {[lindex $spectrumRe $i] == 0} {continue}
                    lset spectrumIm $i [expr {rand() * $pi2}]
                }
            }
        }

        ## build flatness calibration table for interpolation
        set flatTable {}
        foreach freq $options(-flatFreqsList) amp $options(-flatAmpsList) {lappend flatTable $freq $amp}
        unset -nocomplain options(-flatFreqsList)
        unset -nocomplain options(-flatAmpsList)

#        check_events
        ################ build real- and imaginary spectrum list and apply amplitude flatness calibration parameters ################
        ## build real and imaginary spectrum list
        set rmsPower 0
        set iOld -1e9
        set i 0
        set deltaI [expr {round(0.1 / $options(-spacing))}]
        foreach amp $spectrumRe phase $spectrumIm {
            ## calculate rms power by summing the squared linear uncorrected amplitudes
            set rmsPower [expr {$rmsPower + $amp * $amp}]
            if {($i - $iOld) > $deltaI} {
                if {$i < $fftPoints2} {
                    set freq [expr {$options(-center) + $i * $options(-spacing)}]
                } else {
                    set freq [expr {$options(-center) + ($i - $fftPoints) * $options(-spacing)}]
                }
                set flatCorrDb [::math::interpolate::interp-linear $flatTable [expr {$freq + 0.05}]]
                set flatCorrFactor [expr {pow(10, ($flatCorrDb / 20.))}]
                set iOld $i
#                check_events
            }
            ## add the interpolated calibration values to the signal amplitudes and de-log
            set amplCal [expr {$amp * $flatCorrFactor}]
            lset spectrumRe $i [expr {$amplCal * cos($phase)}]
            lset spectrumIm $i [expr {$amplCal * sin($phase)}]
            incr i
        }
#            Log d "calcMCWaveform: amplitude calibration = [expr {([clock clicks -milli]-$startTime)/1000.}]seconds"; set startTime1 [clock clicks -milli]
#            check_events

        ## powerSum is total requested output power in dBm
        set powerSum [expr {10. * log10($rmsPower)}]

        ################ apply sideband suppression calibration parameters #########################################

        ## separate the spectrum in I-spectrum (even) and Q-spectrum (odd) by adding/substracting the mirrored spectrum
        ## then apply the ssb calibration data to the Q spectrum only and rebuild the spectrum with spectrum = spectrumI + i*spectrumQ
        ## spectrumQ = (spectrum + spectrumQMirrored)/2i  spectrumI = (spectrum + spectrumIMirrored)/2
        ## build complete spectrum lists with zeros

        set ssbAmpTable   {}
        set ssbPhaseTable {}
        set degRad [expr {$pi / 180.}]
        ## build ssb correction tables. frequencies are in the range of +/- 260Mhz
        foreach freq $options(-ssbFreqsList) amp $options(-ssbAmpsList) phase $options(-ssbPhaseList) {
            lappend ssbAmpTable $freq [expr {pow(10, $amp / 20.)}]
            lappend ssbPhaseTable $freq [expr {$degRad * $phase}]
        }
        ## entry for fft index = 0: at center frequency, is different from other frequencies
        set ampCorrFactor   [::math::interpolate::interp-linear $ssbAmpTable 0]
        set re 1
        set qre [expr {[lindex $spectrumIm 0] * $re * $ampCorrFactor}]
        ## qim and iim are zero
        set ire [lindex $spectrumRe 0]
        fftw::setAt $plan 1 0 [list $ire  $qre]
        set freqOld -1000.
        set freq 0
        for {set i 1} {$i < $fftPoints} {incr i} {
            set iMirrored [expr {$fftPoints - $i}]
            set qre [expr {([lindex $spectrumIm $i] + [lindex $spectrumIm $iMirrored]) / 2.}]
            set qim [expr {([lindex $spectrumRe $iMirrored] - [lindex $spectrumRe $i]) / 2.}]
            set ire [expr {([lindex $spectrumRe $i] + [lindex $spectrumRe $iMirrored]) / 2.}]
            set iim [expr {([lindex $spectrumIm $i] - [lindex $spectrumIm $iMirrored]) / 2.}]
            if {($qre == 0.) && ($qim == 0.) && ($ire == 0.) && ($iim == 0.)} {continue}
            if {$i < $fftPoints2} {
                set freq [expr {$i * $options(-spacing)}]
            } else {
                set freq [expr {($i - $fftPoints) * $options(-spacing)}]
            }
            ## calculate new ssb corrections only every 1MHz for speed
            if {abs($freq - $freqOld) > 1} {
                set ampCorrFactor [::math::interpolate::interp-linear $ssbAmpTable [expr {$freq + 0.005}]]
                set phaseCorrFactor [::math::interpolate::interp-linear $ssbPhaseTable [expr {$freq + 0.005}]]
                set re [expr {cos($phaseCorrFactor)}]
                set im [expr {sin($phaseCorrFactor)}]
                set freqOld $freq
#                check_events
            }
            set qre [expr  {($qre * $re + $qim * $im) * $ampCorrFactor}]
            set qim [expr  {($qim * $re - $qre * $im) * $ampCorrFactor}]
            fftw::setAt $plan 1 $i [list  [expr {$ire - $qim}] [expr {$iim + $qre}]]
        }
        unset spectrumIm
        unset spectrumRe
        unset ssbAmpTable
        unset ssbPhaseTable
#        Log d "calcMCWaveform: apply ssb calibration = [expr {([clock clicks -milli]-$startTime1)/1000.}]seconds"; set startTime1 [clock clicks -milli]

        fftw::execute $plan 1
#        Log d "calcMCWaveform: IFFT = [expr {([clock clicks -milli]-$startTime1)/1000.}]seconds"; set startTime1 [clock clicks -milli]
        set scaleFactor [fftw::scale $plan 1 32767.]

#        Log d "calcMCWaveform: IFFT scale = [expr {([clock clicks -milli]-$startTime1)/1000.}]seconds"; set startTime1 [clock clicks -milli]

        ## scale the rms value with the same scaling factor
        set rmsPower [expr {10 * log10($rmsPower / pow(32767. / $scaleFactor, 2))}]
#        check_events

        ## return waveform, rms power and total power
        if {[valbool $options(-scale)]} {
            return [list -rms $rmsPower -pattern [fftw::getFlatInt $plan 1 0] -powerSum $powerSum]
        } else {
            ## de-scale pattern, in c it would be shorter
            set patternUnScaled {}
            foreach val [fftw::getFlatInt $plan 1 0] {lappend patternUnScaled [expr {$val / $scaleFactor}]}
            return [list -rms $rmsPower -pattern $patternUnScaled -powerSum $powerSum]
        }

        ## END criple finally in order to deallocate FFTW plan
    } err] == 1} {
        ## abnormal termination with error
        fftw::destroy $plan

        ## rethrow error
        error $err
    } else {
        ## normal termination
        fftw::destroy $plan

        ## return result
        return $err
    }
}
