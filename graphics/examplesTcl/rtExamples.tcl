catch {load vtktcl}
#
# This is a regression test script for VTK.
#

#
# if REG_IMAGE_PATH is defined, then use if to qualify the error and test images
#
if { [catch {set REG_IMAGE_PATH $env(REG_IMAGE_PATH)/}] != 0} { set REG_IMAGE_PATH "" }

# first find all the examples. they can be defined on command line or in
# current directory

if { $argv != ""} {
  set files $argv
  } else {
  set files [lsort [glob {[A-z]*.tcl}]]
}

# remove files that are not suitable for regression tests or simply don't work right now
set noTest {
   rt.tcl rtAll.tcl rib.tcl TkInteractor.tcl TkRenderWidget.tcl RenderWidget.tcl 
   rtExamples.tcl polyViewer.tcl KeyFrame.tcl cameraKey.tcl timing.tcl 
   Decimate.tcl assembly2.tcl connPineRoot.tcl deciHawa.tcl 
   deciPineRoot.tcl deleted.tcl mcTest.tcl viewMCubesFile.tcl
   volTkInteractor.tcl spikeColor.tcl tkwin.tcl 3dsToRIB.tcl backdrop.tcl
   ShotNoiseInclude.tcl TestFeatureAnd.tcl TestHistogram.tcl 
   TestHistogramEqualization.tcl TestMIPFilter.tcl
   TestSubsample3D.tcl
   TestWriter.tcl Timing.tcl WindowLevelInterface.tcl
   vtkImageInclude.tcl}

for {set i 0} {$i < [llength $noTest]} {incr i} {
   if {[set pos [lsearch $files [lindex $noTest $i]]] != -1} {
      set files [lreplace $files $pos $pos ]
   }
}

# now do the tests
foreach afile $files {
   #
   # only tcl scripts with valid/ images are tested
   if {[catch {set channel [open "valid/$afile.ppm"]}] != 0 } {
      puts "WARNING: There is no valid image for $afile"
      continue
   } else {
      close $channel
   }
   
   vtkMath rtExMath
   rtExMath RandomSeed 6
   
   puts -nonewline "$afile took "
   flush stdout
   puts -nonewline "[expr [lindex [time {source $afile} 1] 0] / 1000000.0] seconds "
   
   vtkWindowToImageFilter w2if
   
   # look for a renderWindow ImageWindow or ImageViewer
   # first check for some common names
   if {[info commands renWin] == "renWin"} {
      w2if SetInput renWin
      set threshold 10
   } else {
      set threshold 0
      if {[info commands viewer] == "viewer"} {
	 w2if SetInput [viewer GetImageWindow]
	 viewer Render
      } else {
	 if {[info commands imgWin] == "imgWin"} {
	    w2if SetInput imgWin
	    imgWin Render
	 }
      }
   }

   # run the event loop quickly to map any tkwidget windows
   wm withdraw .
   update

   if {[catch {set channel [open "valid/$afile.ppm"]}] != 0 } {
      puts "\nWARNING: Creating a valid image for $afile"
      vtkPNMWriter rtpnmw
      rtpnmw SetInput [w2if GetOutput]
      rtpnmw SetFileName "valid/$afile.ppm"
      rtpnmw Write
      vtkCommand DeleteAllObjects
      catch {destroy .top}
      continue
   }
   close $channel
   
   vtkPNMReader rtpnm
   rtpnm SetFileName "valid/$afile.ppm"
   
   vtkImageDifference imgDiff
   
   if {$threshold == 0} {imgDiff AllowShiftOff; imgDiff SetThreshold 0}
   imgDiff SetInput [w2if GetOutput]
   imgDiff SetImage [rtpnm GetOutput]
   imgDiff Update

   # a test has to be off by at least threshold pixels for us to care   
   if {[imgDiff GetThresholdedError] <= $threshold} {
       puts "and Passed"
   } else {
       puts "but failed with an error of [imgDiff GetThresholdedError]"
         vtkPNMWriter rtpnmw
         rtpnmw SetInput [imgDiff GetOutput]
       rtpnmw SetFileName "${REG_IMAGE_PATH}$afile.error.ppm"
         rtpnmw Write
         vtkPNMWriter rtpnmw2
         rtpnmw2 SetInput [w2if GetOutput]
       rtpnmw2 SetFileName "${REG_IMAGE_PATH}$afile.test.ppm"
         rtpnmw2 Write
   }
   
   vtkCommand DeleteAllObjects
   catch {destroy .top}
}

exit