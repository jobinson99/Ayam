# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2008 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# win.tcl - (top level) window management


##############################
# winToMouse:
# place window w on screen near the mouse pointer
proc winToMouse { w } {

    wm withdraw $w
    update idletasks

    set x [expr [winfo pointerx $w] + 20]
    set y [expr [winfo pointery $w] + 20]

    winMoveOrResize $w "+${x}+${y}"

    wm deiconify $w

 return;
}
# winToMouse


##############################
# winCenter:
# center window w on screen (floating windows gui mode) or in the
# middle of the main window (single window gui mode)
# or in the middle of the provided parent window (p);
# if p is "-" w is centered on the screen
proc winCenter { w {p "."} } {
    global ayprefs

    wm withdraw $w
    update idletasks

    if { ($p != "-") && $ayprefs(SingleWindow) } {
	# center over window (p)
	set x [expr ([winfo rootx $p] + [winfo width $p]/2) \
		   - [winfo reqwidth $w]/2]
	set y [expr ([winfo rooty $p] + [winfo height $p]/2) \
		   - [winfo reqheight $w]/2]
    } else {
	# center on screen
	set x [expr [winfo screenwidth $w]/2 - [winfo reqwidth $w]/2]
	set y [expr [winfo screenheight $w]/2 - [winfo reqheight $w]/2]
    }

    winMoveOrResize $w "+${x}+${y}"

    wm deiconify $w

 return;
}
# winCenter


##############################
# winRestoreOrCenter:
#  restore geometry or center the window
proc winRestoreOrCenter { w t {c "."} } {
    global ay ayprefs aygeom

    regsub -all "\[\[:space:\]\[:cntrl:\]\]" $t "_" st
    set var aygeom([string range $w 1 end]_${st})
    set geom ""
    catch "set geom \$$var"

    if { ($ayprefs(SaveDialogGeom) > 0) } {
	if { ($geom != "") } {
	    winMoveOrResize $w $geom
	} else {
	    winCenter $w $c
	}
	bind $w <Configure>\
	    "if { \"%W\" == \"$w\" } { set $var \[winGetGeom $w\] }"
    } else {
	winCenter $w $c
    }

    wm deiconify $w

 return;
}
# winRestoreOrCenter


##############################
# winMoveOrResize:
#  move or resize window w according to newgeom
proc winMoveOrResize { w newgeom } {
    global ay ayprefs

    if { $newgeom == "" } { return }

    update idletasks
    set oldgeom [wm geom $w]

    regexp {([0-9]+)?x?([0-9]+)?(\+)?([0-9\-]+)?(\+)?([0-9\-]+)?}\
	$oldgeom blurb width height blurb2 x blurb3 y

    regexp {([0-9]+)?x?([0-9]+)?(\+)?([0-9\-]+)?(\+)?([0-9\-]+)?}\
	$newgeom blurb nw nh blurb2 nx blurb3 ny

    if { $nw == "" } {
	# no new width specified => move the window only
	if { $nx != "" } { set x $nx }
	if { $ny != "" } { set y $ny }

	# never move off-screen
	set mw [lindex [wm maxsize .] 0]
	set mh [lindex [wm maxsize .] 1]
	if { $mw <= [winfo screenwidth $w] &&
	     $mh <= [winfo screenheight $w] } {
	    # single monitor setup
	    if { [expr abs($x)] >= [winfo screenwidth $w] } {
		set x 20
	    }
	    if { [expr abs($y)] >= [winfo screenheight $w] } {
		set y 20
	    }
	} else {
	    # multi monitor setup
	    # move to main window as fallback
	    if { [expr abs($x)] >= $mw } {
		set x [expr [winfo rootx .] + 20]
	    }
	    if { [expr abs($y)] >= $mh } {
		set y [expr [winfo rooty .] + 20]
	    }
	}

	set newgeom ""
	append newgeom "+$x"
	append newgeom "+$y"
	catch {wm geom $w $newgeom}
    } else {
	# new width specified => resize
	if { $nx == "" } {
	    # no new position specified => only resize
	    if { $nw != "" } { set width $nw }
	    if { $nh != "" } { set height $nh }

	    if { ($ayprefs(TwmCompat) == 0) &&
		 ($ay(ws) != "Windows") } {
		set x [winfo rootx $w]
		set y [winfo rooty $w]
		set newgeom "${width}x${height}"
		append newgeom "+$x"
		append newgeom "+$y"
	    } else {
		set newgeom "${width}x${height}"
	    }
	    catch {wm geom $w $newgeom}
	} else {
	    # new width and position specified
	    if { $nx != "" } { set x $nx }
	    if { $ny != "" } { set y $ny }
	    if { $nw != "" } { set width $nw }
	    if { $nh != "" } { set height $nh }

	    # never move off screen
	    set mw [lindex [wm maxsize .] 0]
	    set mh [lindex [wm maxsize .] 1]
	    if { $mw <= [winfo screenwidth $w] &&
		 $mh <= [winfo screenheight $w] } {
		# single monitor setup
		if { [expr abs($x)] >= [winfo screenwidth $w] } {
		    set x 20
		}
		if { [expr abs($y)] >= [winfo screenheight $w] } {
		    set y 20
		}
	    } else {
		# multi monitor setup
		# move to main window as fallback
		if { [expr abs($x)] >= $mw } {
		    set x [expr [winfo rootx .] + 20]
		}
		if { [expr abs($y)] >= $mh } {
		    set y [expr [winfo rooty .] + 20]
		}
	    }

	    set newgeom "${width}x${height}"
	    append newgeom "+$x"
	    append newgeom "+$y"

	    catch {wm geom $w $newgeom}
	}
	# if
    }
    # if

    update idletasks

 return;
}
# winMoveOrResize


##############################
# winIconWindow:
#  set icon window for window w
proc winIconWindow { w img } {

#image create photo icon_img -format GIF -file ./ayam-icon-t.gif

    if { ! [winfo exists .icon] } {
	toplevel .icon
	pack [label .icon.l -image $img]
    }

    wm iconwindow $w .icon
    wm withdraw $w
    wm deiconify $w

 return;
}
# winIconWindow


##############################
# winGetGeom:
#  get geometry of window w ready to be fed into a call to wm geom
#  to re-restablish this geometry; TwmCompat controls, whether the
#  window manager (Unix/X11) adds the decoration to wm geom information
#  forcing us to use winfo instead.
proc winGetGeom { w } {
    global tcl_platform ayprefs

    if { ($tcl_platform(platform) != "windows") &&\
	    ($ayprefs(TwmCompat) != 1) } {
	set x [winfo rootx $w]
	set y [winfo rooty $w]
	set wi [winfo width $w]
	set he [winfo height $w]

	set ng ""
	append ng "${wi}x${he}"
	if { $x >= 0 } { append ng "+$x" } else { append ng "-$x" }
	if { $y >= 0 } { append ng "+$y" } else { append ng "-$y" }
	return $ng
    } else {
	return [wm geom $w]
    }

}
# winGetGeom


##############################
# winAutoFocusOff:
#  disable AutoFocus
proc winAutoFocusOff { } {
    global ay ayprefs

    if { $ayprefs(SafeAutoFocus) } {
	if { !$ay(afdisabled) } {
	    set ay(oldAutoFocus) $ayprefs(AutoFocus)
	    set ayprefs(AutoFocus) 0
	}
	incr ay(afdisabled)
    }

 return;
}
# winAutoFocusOff


##############################
# winAutoFocusOn:
#  (re-)enable AutoFocus
proc winAutoFocusOn { } {
    global ay ayprefs

    if { $ayprefs(SafeAutoFocus) } {
	# decr ay(afdisabled)
	incr ay(afdisabled) -1
	if { $ay(afdisabled) == 0 } {
	    set ayprefs(AutoFocus) $ay(oldAutoFocus)
	}
    }

 return;
}
# winAutoFocusOn


##############################
# winSetWMState:
#  restore the window manager state for window w
#  since we only restore the zoomed state presently, this
#  proc should rather be called winSetZoomedState
proc winSetWMState { w state } {
 global ayprefs tcl_patchLevel

    if { $state != [wm state $w] } {
	if { $tcl_patchLevel > "8.3" } {
	    catch { wm state $w $state }
	} else {
	    # we can not exactly restore the state in pre 8.4, but we can
	    # at least try to restore the size and place for the zoomed state
	    if { $state == "zoomed" } {
		set maxsize [wm maxsize .]
		set maxw [lindex $maxsize 0]

		regexp \
		  {([0-9]+)?x?([0-9]+)?(\+)?([0-9\-]+)?(\+)?([0-9\-]+)?} \
		  $ayprefs(mainGeom) blurb nw nh blurb2 nx blurb3 ny

		set border [expr $maxw - [winfo screenwidth $w]]

		catch {wm geometry $w [winfo screenwidth .]x${nh}+-${border}+0}
	    }
	}
    }

 return;
}
# winSetWMState


##############################
# winRestorePaneLayout:
#  restore the pane layout for SingleWindow GUI mode
proc winRestorePaneLayout { config } {
    global ayprefs

    if {[llength $config] > 6} {
	set minx [winfo rootx .]
	set miny [winfo rooty .]
	set maxx [expr $minx + [winfo width .]]
	set maxy [expr $miny + [winfo height .]]

	if { [lindex $config 2] < $miny ||
	     [lindex $config 2] > $maxy ||
	     [lindex $config 3] < $miny ||
	     [lindex $config 3] > $maxy ||
	     [lindex $config 4] < $minx ||
	     [lindex $config 4] > $maxx ||
	     [lindex $config 5] < $minx ||
	     [lindex $config 5] > $maxx ||
	     [lindex $config 6] < $minx ||
	     [lindex $config 6] > $maxx } {
	    set config ""
	}
    }
    # from bottom to top
    # horizontal pane between console and hierarchy
    if {[llength $config] > 3} {
	set vheight [lindex $config 3]
    } else {
	set vheight [expr [winfo rooty .] + \
			 ([winfo height .] - [winfo reqheight .fl])]
    }
    pane_constrain . .__h2 .fu .fl height y 0\
     [lindex $ayprefs(PaneMargins) 0]
    pane_motion $vheight . .__h2 height y 1
    update idletasks

    # horizontal pane between hierarchy and internal views
    if {[llength $config] > 2} {
	set vheight [lindex $config 2]
    } else {
	set vheight [expr [winfo rooty .] + \
			 (([winfo height .] - [winfo height .fl])*0.62)]
    }
    pane_constrain . .__h1 .fv .fu height y 0\
     [lindex $ayprefs(PaneMargins) 1]
    pane_motion $vheight . .__h1 height y 1
    update idletasks

    # from right to left
    # vertical pane between internal properties and internal view3
    if {[llength $config] > 5} {
	set vwidth [lindex $config 5]
    } else {
	set vwidth [expr 5+[winfo rootx .fu]+([winfo width .fu]*0.6)]
    }
    pane_constrain . .fu.fMain.__h2 .fu.fMain.fProp .fu.fMain.fview3\
     width x 0 [lindex $ayprefs(PaneMargins) 2]
    pane_motion $vwidth . .fu.fMain.__h2 width x 1
    update idletasks

    # vertical pane between hierarchy and internal properties
    if {[llength $config] > 4} {
	set vwidth [lindex $config 4]
    } else {
	set vwidth [expr 5+[winfo rootx .fu]+([winfo width .fu]*0.18)]
    }
    pane_constrain . .fu.fMain.__h1 .fu.fMain.fHier .fu.fMain.fProp\
     width x 1 [lindex $ayprefs(PaneMargins) 3]
    pane_motion $vwidth . .fu.fMain.__h1 width x 1
    update idletasks

    # vertical pane between the internal views 1 and 2
    if {[llength $config] > 6} {
	set vwidth [lindex $config 6]
    } else {
	set vwidth [expr 5+[winfo rootx .fv]+([winfo width .fv]*0.5)]
    }
    pane_constrain . .fv.fViews.__h1 .fv.fViews.fview1 .fv.fViews.fview2 \
     width x 0 [lindex $ayprefs(PaneMargins) 4]
    pane_motion $vwidth . .fv.fViews.__h1 width x 1
    update idletasks

 return;
}
# winRestorePaneLayout


##############################
# winGetPaneLayout:
#  returns the current pane layout for SingleWindow GUI mode
proc winGetPaneLayout { } {

    set config ""
# should use [winfo vrootwidth .]
    lappend config [winfo screenwidth .]
    lappend config [winfo screenheight .]
    lappend config [expr [winfo rooty .__h1] + \
				     [winfo height .__h1]]
    lappend config [expr [winfo rooty .__h2] + \
				     [winfo height .__h2]]
    lappend config [expr [winfo rootx .fu.fMain.__h1] + \
				     [winfo width .fu.fMain.__h1]]
    lappend config [expr [winfo rootx .fu.fMain.__h2] + \
				     [winfo width .fu.fMain.__h2]]
    lappend config [expr [winfo rootx .fv.fViews.__h1] + \
				     [winfo width .fv.fViews.__h1]]

 return $config;
}
# winGetPaneLayout


##############################
# winMakeFloat:
#  make window <w> a floating window on MacOSX Aqua
#  (similar to transient windows on X11)
proc winMakeFloat { w } {

    ::tk::unsupported::MacWindowStyle style $w floating	{closeBox resizable}

 return;
}
# winMakeFloat


##############################
# winDialog:
#  create a dialog window
#  the window will immediately be withdrawn so that population, initial
#  layout, and movement to final position on screen do not distract the user;
#  after population of the dialog any of winRestoreOrCenter, winCenter, or
#  winToMouse will then deiconify the window again
proc winDialog { w t } {
 global ay ayprefs

    catch {destroy $w}
    toplevel $w -class Ayam
    wm title $w $t
    wm iconname $w "Ayam"
    if { $ay(ws) == "Aqua" } {
	winMakeFloat $w
    } else {
	wm transient $w .
    }

    wm withdraw $w

 return;
}
# winDialog

##############################
# winOpenPopup:
#  open the popup menu named "popup" for a given window w
#  near the current mouse pointer position
proc winOpenPopup { w } {
    tk_popup $w.popup [winfo pointerx $w] [winfo pointery $w]
}
