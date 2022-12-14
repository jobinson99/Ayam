# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2005 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# tgui.tcl - Tesselation GUI Tcl code

uplevel #0 { array set tgui_tessparam {
    SamplingMethod 0
    SamplingParamU 0.25
    SamplingParamV 0.25

    SamplingParamU1 0.25
    SamplingParamV1 0.25

    SamplingParamU2 1.5
    SamplingParamV2 1.5

    SamplingParamU3 8
    SamplingParamV3 8

    SamplingParamU4 10
    SamplingParamV4 10

    SamplingParamU5 1
    SamplingParamV5 1

    SamplingParamU6 3
    SamplingParamV6 3

    UseTexCoords 0
    UseVertColors 0
    UseVertNormals 0

    RefineTrims 0
    Primitives 0
    QuadEps 5.0

    LazyUpdate 0

    MB1Down 0
    ReadTag 0
    OldSamplingMethod -1
    OldSamplingParamU -1
    OldSamplingParamV -1

    NumPolys 0

    updatelock 0
}
array set tgui_tessparamdefaults [array get tgui_tessparam]
}

# tgui_block:
#  block user interactions in main and toolbox windows that could
#  destroy objects
proc tgui_block { blockMsg } {
    global ay ayprefs

    set ay(ButtonBinding) [bind Button <1>]
    set sc "if { (\[winfo toplevel %W\] == \".\") || \
	    (\[winfo toplevel %W\] == \".tbw\") } {\
	    ayError 2 Ayam \"$blockMsg\"; bell;\
	    break } else { eval {$ay(ButtonBinding)} }"
    bind Button <1> $sc

    set ay(MenubuttonBinding) [bind Menubutton <1>]
    set sc "if { \[winfo toplevel %W\] == \".\" } {\
	    ayError 2 Ayam \"$blockMsg\"; bell;\
	    break } else { eval {$ay(MenubuttonBinding)} }"
    bind Menubutton <1> $sc

    set ay(ListboxBinding) [bind Listbox <1>]
    set sc "if { \[winfo toplevel %W\] == \".\" } {\
	    ayError 2 Ayam \"$blockMsg\"; bell;\
	    break } else { eval {$ay(ListboxBinding)} }"
    bind Listbox <1> $sc

    set ay(ListboxRBinding) [bind Listbox <ButtonRelease-1>]
    set sc "if { \[winfo toplevel %W\] == \".\" } {\
	    ayError 2 Ayam \"$blockMsg\"; bell;\
	    break } else { eval {$ay(ListboxRBinding)} }"
    bind Listbox <ButtonRelease-1> $sc

    set sc "ayError 2 Ayam \"$blockMsg\"; bell; break"

    # unbind Objects label and object tree or object listbox
    if { $ay(lb) == 0 } {
	set ay(oldlabelbinding) [bind .fu.fMain.fHier.ftr.la <Double-1>]
	bind .fu.fMain.fHier.ftr.la <Double-1> $sc
	set t $ay(tree)

	set ay(oldtreeb1binding) [bind $t <1>]
	bind $t <1> $sc
	set ay(oldtreeb1rbinding) [bind $t <ButtonRelease-1>]
	bind $t <ButtonRelease-1> $sc

	set ay(oldtreeb2binding) [bind $t <2>]
	bind $t <2> $sc

	set ay(oldtreeb3binding) [bind $t <3>]
	bind $t <3> $sc

	$t bindText <ButtonRelease-1> ""
	$t bindText <ButtonPress-1> ""
    } else {
	set ay(oldlabelbinding) [bind .fu.fMain.fHier.flb.la <Double-1>]
	bind .fu.fMain.fHier.flb.la <Double-1> $sc
    }

    # unbind property listbox
    set ay(oldplbb1rbinding) [bind $ay(plb) <ButtonRelease-1>]
    bind $ay(plb) <ButtonRelease-1> $sc

    # unbind toolbox and main window via a new bindtag "tguinone"
    bind tguinone <Control-KeyPress> $sc
    bind tguinone <KeyPress> $sc

    if { [winfo exists .tbw] } {
	bindtags .tbw {tguinone .tbw ayam all}
    }
    bindtags .fl.con {tguinone .fl.con . all}
    bindtags . {tguinone . Ayam all}

    set sc "if { \[winfo toplevel %W\] == \".\" } {\
	    ayError 2 Ayam \"$blockMsg\"; bell;\
	    break } else { continue }"

    bind Frame <Control-KeyPress> $sc
    bind Canvas <Control-KeyPress> $sc
    bind Console <Control-KeyPress> $sc
    bind Listbox <Control-KeyPress> $sc

    winAutoFocusOff

    # modify mouse cursor to a watch
    mouseWatch 1 {. .tbw}

 return;
}
# tgui_block


# tgui_unblock:
#  unblock user interactions in main and toolbox windows that could
#  destroy objects
proc tgui_unblock { } {
    global ay ayprefs

    bind Button <1> $ay(ButtonBinding)
    bind Menubutton <1> $ay(MenubuttonBinding)
    bind Listbox <1> $ay(ListboxBinding)
    bind Listbox <ButtonRelease-1> $ay(ListboxRBinding)

    if { $ay(lb) == 0 } {
	# bind Objects label
	bind .fu.fMain.fHier.ftr.la <Double-1> $ay(oldlabelbinding)
	# re-establish tree bindings
	set t $ay(tree)
	bind $t <1> $ay(oldtreeb1binding)
	bind $t <ButtonRelease-1> $ay(oldtreeb1rbinding)
	bind $t <2> $ay(oldtreeb2binding)
	bind $t <3> $ay(oldtreeb3binding)
	$t bindText <ButtonRelease-1> ""
	$t bindText <ButtonPress-1> "tree_selectItem $t"
    } else {
	bind .fu.fMain.fHier.flb.la <Double-1> $ay(oldlabelbinding)
    }

    bind $ay(plb) <ButtonRelease-1> $ay(oldplbb1rbinding)

    if { [winfo exists .tbw] } {
	bindtags .tbw {.tbw ayam all}
	bind tguinone <Control-KeyPress> ""
    }

    bindtags .fl.con {.fl.con . all}
    bindtags . {. Ayam all}

    bind Frame <Control-KeyPress> ""
    bind Canvas <Control-KeyPress> ""
    bind Console <Control-KeyPress> ""
    bind Listbox <Control-KeyPress> ""

    winAutoFocusOn

    # reset mouse cursor
    mouseWatch 0 {. .tbw}

 return;
}
# tgui_unblock


# tgui_update:
#
#
proc tgui_update args {
    global ay ayprefs tgui_tessparam

    if { $tgui_tessparam(updatelock) == 1 } {
	return;
    }

    set tgui_tessparam(updatelock) 1

    # only need to check changes when called from the slider,
    # and not when called from a variable trace
    set checkChanges 0
    if { [llength $args] == 1 } {
	set checkChanges 1
    }

    tgui_deletetraces

    set i [expr $tgui_tessparam(SamplingMethod) + 1]

    if { $tgui_tessparam(OldSamplingMethod) !=
	 $tgui_tessparam(SamplingMethod) } {
	if { $tgui_tessparam(SamplingMethod) == 0 } {
	    .tguiw.f1.fSamplingParamU.ll conf -text "0"
	    .tguiw.f1.fSamplingParamU.lr conf -text "1"
	    .tguiw.f1.fSamplingParamU.s conf -from 0 -to 1
	    .tguiw.f1.fSamplingParamU.s conf -resolution 0.05
	    .tguiw.f1.fSamplingParamV.s conf -state disabled
	    .tguiw.f1.fSamplingParamV.e conf -state disabled
	}
	if { $tgui_tessparam(SamplingMethod) == 1 } {
	    .tguiw.f1.fSamplingParamU.ll conf -text "0"
	    .tguiw.f1.fSamplingParamU.lr conf -text "5"
	    .tguiw.f1.fSamplingParamU.s conf -from 0 -to 5
	    .tguiw.f1.fSamplingParamU.s conf -resolution 0.1
	    .tguiw.f1.fSamplingParamV.s conf -state disabled
	    .tguiw.f1.fSamplingParamV.e conf -state disabled
	}
	if { ($tgui_tessparam(SamplingMethod) == 2) ||
	     ($tgui_tessparam(SamplingMethod) == 3) } {
	    .tguiw.f1.fSamplingParamU.ll conf -text "0"
	    .tguiw.f1.fSamplingParamU.lr conf -text "20"
	    .tguiw.f1.fSamplingParamU.s conf -from 0 -to 20
	    .tguiw.f1.fSamplingParamV.ll conf -text "0"
	    .tguiw.f1.fSamplingParamV.lr conf -text "20"
	    .tguiw.f1.fSamplingParamV.s conf -from 0 -to 20
	}
	if { ($tgui_tessparam(SamplingMethod) == 4) ||
	     ($tgui_tessparam(SamplingMethod) == 5) } {
	    .tguiw.f1.fSamplingParamU.lr conf -text "10"
	    .tguiw.f1.fSamplingParamU.s conf -from 0 -to 10
	    .tguiw.f1.fSamplingParamV.lr conf -text "10"
	    .tguiw.f1.fSamplingParamV.s conf -from 0 -to 10
	}
	if { ($tgui_tessparam(SamplingMethod) > 1) } {
	    .tguiw.f1.fSamplingParamU.s conf -resolution 0.1
	    .tguiw.f1.fSamplingParamV.s conf -resolution 0.1
	    .tguiw.f1.fSamplingParamV.s conf -state normal
	    .tguiw.f1.fSamplingParamV.e conf -state normal
	}
	eval "set tgui_tessparam(SamplingParamU)\
                   \$tgui_tessparam(SamplingParamU$i)"
	eval "set tgui_tessparam(SamplingParamV)\
                   \$tgui_tessparam(SamplingParamV$i)"
    } else {
	eval "set tgui_tessparam(SamplingParamU$i)\
                   \$tgui_tessparam(SamplingParamU)"
	eval "set tgui_tessparam(SamplingParamV$i)\
                   \$tgui_tessparam(SamplingParamV)"
    }

    set doTess 0
    if { $checkChanges } {
	if { ( $tgui_tessparam(OldSamplingMethod) !=
	       $tgui_tessparam(SamplingMethod) ) ||
	     ( $tgui_tessparam(OldSamplingParamU) !=
	       $tgui_tessparam(SamplingParamU) ) ||
	     ( $tgui_tessparam(OldSamplingParamV) !=
	       $tgui_tessparam(SamplingParamV) ) } {
	    if { $tgui_tessparam(LazyUpdate) } {
		if { $tgui_tessparam(MB1Down) == 0 } { set doTess 1 }
	    } else {
		set doTess 1
	    }
	}
    } else {
	if { $tgui_tessparam(LazyUpdate) } {
	    if { $tgui_tessparam(MB1Down) == 0 } { set doTess 1 }
	} else {
	    set doTess 1
	}
    }

    if { $doTess } {
	tguiCmd up\
	    $tgui_tessparam(SamplingMethod) $tgui_tessparam(SamplingParamU)\
	    $tgui_tessparam(SamplingParamV) $tgui_tessparam(UseTexCoords)\
	    $tgui_tessparam(UseVertColors) $tgui_tessparam(UseVertNormals)\
	    $tgui_tessparam(RefineTrims) $tgui_tessparam(Primitives)\
	    $tgui_tessparam(QuadEps)
    }

    set tgui_tessparam(OldSamplingMethod) $tgui_tessparam(SamplingMethod)
    set tgui_tessparam(OldSamplingParamU) $tgui_tessparam(SamplingParamU)
    set tgui_tessparam(OldSamplingParamV) $tgui_tessparam(SamplingParamV)

    .tguiw.f1.fSamplingParamU.e delete 0 end
    .tguiw.f1.fSamplingParamU.e insert 0 $tgui_tessparam(SamplingParamU)
    .tguiw.f1.fSamplingParamV.e delete 0 end
    .tguiw.f1.fSamplingParamV.e insert 0 $tgui_tessparam(SamplingParamV)

    tgui_establishtraces

    set tgui_tessparam(updatelock) 0

 return;
}
# tgui_update


proc tgui_deletetraces { } {
    global tgui_tessparam

    trace vdelete tgui_tessparam(SamplingMethod) w tgui_update
    #trace vdelete tgui_tessparam(SamplingParamU) w tgui_update
    #trace vdelete tgui_tessparam(SamplingParamV) w tgui_update

    trace vdelete tgui_tessparam(UseTexCoords) w tgui_update
    trace vdelete tgui_tessparam(UseVertColors) w tgui_update
    trace vdelete tgui_tessparam(UseVertNormals) w tgui_update
    trace vdelete tgui_tessparam(RefineTrims) w tgui_update
    trace vdelete tgui_tessparam(Primitives) w tgui_update

 return;
}

proc tgui_establishtraces { } {
    global tgui_tessparam

    trace variable tgui_tessparam(SamplingMethod) w tgui_update
    #trace variable tgui_tessparam(SamplingParamU) w tgui_update
    #trace variable tgui_tessparam(SamplingParamV) w tgui_update

    trace variable tgui_tessparam(UseTexCoords) w tgui_update
    trace variable tgui_tessparam(UseVertColors) w tgui_update
    trace variable tgui_tessparam(UseVertNormals) w tgui_update
    trace variable tgui_tessparam(RefineTrims) w tgui_update
    trace variable tgui_tessparam(Primitives) w tgui_update

 return;
}


# tgui_recalcslider:
#  re-calculate resolution of slider according to val
#
proc tgui_recalcslider { slider val } {

    if { $slider == 0 } {
	set rmin [.tguiw.f1.fSamplingParamU.s cget -from]
	if { $val < $rmin } {
	    .tguiw.f1.fSamplingParamU.s conf -from $val
	    .tguiw.f1.fSamplingParamU.ll conf -text $val
	    set rmin $val
	}
	set rmax [.tguiw.f1.fSamplingParamU.s cget -to]
	if { $val > $rmax } {
	    .tguiw.f1.fSamplingParamU.s conf -to $val
	    .tguiw.f1.fSamplingParamU.lr conf -text $val
	    set rmax $val
	}
    } else {
	set rmin [.tguiw.f1.fSamplingParamV.s cget -from]
	if { $val < $rmin } {
	    .tguiw.f1.fSamplingParamV.s conf -from $val
	    .tguiw.f1.fSamplingParamV.ll conf -text $val
	    set rmin $val
	}
	set rmax [.tguiw.f1.fSamplingParamV.s cget -to]
	if { $val > $rmax } {
	    .tguiw.f1.fSamplingParamV.s conf -to $val
	    .tguiw.f1.fSamplingParamV.lr conf -text $val
	    set rmax $val
	}
    }

    if { [expr (abs(int($val) - $val)) != 0.0] } {
	set ta [expr abs(int($val) - $val)]
	set tb 1.0
	set done 0
	while { $done != 1 } {
	    set tb [expr $tb/10.0]
	    set ta [expr (abs(int($ta*10.0) - ($ta*10.0)))]
	    if { $ta <= 0.0 } { set done 1 }
	}
	# while
    } else {
	if { [expr abs($rmin - $rmax)] <= 1.0 } {
	    set tb 0.1
	} else {
	    set tb 1.0
	}
    }
    # if

    if { $slider == 0 } {
	.tguiw.f1.fSamplingParamU.s configure -resolution $tb
    } else {
	.tguiw.f1.fSamplingParamV.s configure -resolution $tb
    }

 return;
}
# tgui_recalcslider


# tgui_addtag:
#
#
proc tgui_addtag { } {
    global tgui_tessparam

    undo save AddTPTag
    set tgui_tessparam(tagval) \
	[format "%d,%g,%g,%d,%d" [expr $tgui_tessparam(SamplingMethod) + 1]\
	     $tgui_tessparam(SamplingParamU) $tgui_tessparam(SamplingParamV)\
	     $tgui_tessparam(RefineTrims) $tgui_tessparam(Primitives)]

    forAll -recursive 0 {
	global tgui_tessparam

	set tagnames ""
	set tagvals ""
	getTags tagnames tagvals
	if { ($tagnames == "" ) || ([lsearch -exact $tagnames "TP"] == -1) } {
	    # no tags, or TP tag not present already => just add a new tag
	    addTag "TP" $tgui_tessparam(tagval)
	} else {
	    # we have already a TP tag => change its value
	    set index [lsearch -exact $tagnames "TP"]
	    setTags -index $index "TP" $tgui_tessparam(tagval)
	}
	# if
    }
    # forAll

 return;
}
# tgui_addtag


# tgui_remtag:
#  remove TP tags from all selected NURBS patch objects
#
proc tgui_remtag { } {

    forAll -recursive 0 {
	delTags "TP"
    }
    # forAll

 return;
}
# tgui_remtag


# tgui_readtag:
#  read TP tag from first selected object and set tesselation gui
#  parameters appropriately
proc tgui_readtag { } {
    global tgui_tessparam

    set tgui_tessparam(ReadTag) 0

    set tagnames ""
    set tagvals ""
    getTags tagnames tagvals
    if { ($tagnames != "" ) } {
	set index [lsearch -exact $tagnames "TP"]
	if { $index != -1 } {
	    set val [lindex $tagvals $index]

	    set smethod 0
	    set sparamu 20
	    set sparamv 20
	    set rtrims 0
	    set rprims 0

	    scan $val "%d,%g,%g,%d,%d" smethod sparamu sparamv rtrims rprims

	    set tgui_tessparam(FT${smethod}) 0

	    set smethod [expr $smethod - 1]

	    tgui_recalcslider 0 $sparamu
	    tgui_recalcslider 1 $sparamv

	    set tgui_tessparam(OldSamplingMethod) $smethod
	    set tgui_tessparam(SamplingMethod) $smethod
	    set tgui_tessparam(SamplingParamU) $sparamu
	    set tgui_tessparam(SamplingParamV) $sparamv
	    set tgui_tessparam(RefineTrims) $rtrims
	    set tgui_tessparam(Primitives) $rprims

	    set tgui_tessparam(ReadTag) 1
	}
        # if
    }
    # if

 return;
}
# tgui_readtag


# tgui_open:
#  create tesselation gui
#
proc tgui_open { } {
    global ay ayprefs ay_error tgui_tessparam

    winAutoFocusOff

    # deselect property
    $ay(plb) selection clear 0 end
    plb_update

    undo save Tesselate

    set w .tguiw
    set t "Tesselation"
    winDialog $w $t

    tgui_deletetraces

    set ay(bca) $w.f2.bca
    set ay(bok) $w.f2.bok

    set f [frame $w.f1]
    pack $f -in $w -side top -fill x

    set tgui_tessparam(LazyUpdate) $ayprefs(LazyNotify)

    addText $f t1 "Tesselation Parameters"
    addCommand $f c1 "Reset All!" {
	global tgui_tessparam tgui_tessparamdefaults
	tgui_deletetraces
	set rt $tgui_tessparam(ReadTag)
	array set tgui_tessparam [array get tgui_tessparamdefaults]
	set tgui_tessparam(ReadTag) $rt
	tgui_update
	if { ! [info exists tgui_tessparam(firstreset)] } {
	    after idle {
		tgui_deletetraces
		array set tgui_tessparam [array get tgui_tessparamdefaults]
		set tgui_tessparam(ReadTag) $rt
		tgui_update
		set tgui_tessparamdefaults(firstreset) 1
	    }
	}
    }
    addCheck $f tgui_tessparam LazyUpdate
    addCheck $f tgui_tessparam UseTexCoords
    addCheck $f tgui_tessparam UseVertColors
    addCheck $f tgui_tessparam UseVertNormals
    addParam $f tgui_tessparam RefineTrims {0 1 2 3 4 5}
    addMenu $f tgui_tessparam Primitives\
	{"Triangles" "TrianglesAndQuads" "Quads" "QuadrangulatedTriangles"}

    # SamplingMethod
    addMenu $f tgui_tessparam SamplingMethod $ay(smethods)

    # SamplingParamU
    set f [frame $f.fSamplingParamU -relief sunken -borderwidth 1]
    label $f.l -text "SamplingParamU:" -width 14

    label $f.ll -text "0"
    scale $f.s -showvalue 0 -orient h -from 0 -to 100\
	-highlightthickness 1

    bind $f.s <ButtonPress-1> "set tgui_tessparam(MB1Down) 1;focus %W"
    bind $f.s <ButtonRelease-1> {
	set tgui_tessparam(MB1Down) 0;
	if { $tgui_tessparam(LazyUpdate) } {
	    tgui_update
	}
    }

    label $f.lr -text "100"
    entry $f.e -width 5

    pack $f.l -in $f -side left -fill x -expand no
    pack $f.ll -in $f -side left -expand no
    pack $f.s -in $f -side left -fill x -expand yes
    pack $f.lr -in $f -side left -expand no
    pack $f.e -in $f -side right -fill x -expand yes -padx 2

    pack $f -in $w.f1 -side top -fill x -expand yes

    # SamplingParamV
    set f $w.f1
    set f [frame $f.fSamplingParamV -relief sunken -borderwidth 1]
    label $f.l -text "SamplingParamV:" -width 14

    label $f.ll -text "0"
    scale $f.s -showvalue 0 -orient h -from 0 -to 100\
	-highlightthickness 1
    bind $f.s <ButtonPress-1> "set tgui_tessparam(MB1Down) 1;focus %W"
    bind $f.s <ButtonRelease-1> {
	set tgui_tessparam(MB1Down) 0;
	if { $tgui_tessparam(LazyUpdate) } {
	    tgui_update
	}
    }

    label $f.lr -text "100"
    entry $f.e -width 5

    pack $f.l -in $f -side left -fill x -expand no
    pack $f.ll -in $f -side left -expand no
    pack $f.s -in $f -side left -fill x -expand yes
    pack $f.lr -in $f -side left -expand no
    pack $f.e -in $f -side right -fill x -expand yes -padx 2

    pack $f -in $w.f1 -side top -fill x -expand yes

    set f $w.f1
    addText $f t2 "Tesselation Results"
    addInfo $f tgui_tessparam NumPolys

    uie_setLabelWidth $f 16

    # set up undo system
    set ::ay(need_undo_clear) 0
    forAll -recursive 0 { if { [hasChild] } { set ::ay(need_undo_clear) 1 } }

    # read preferences from eventually present TP tag
    tgui_readtag

    # remove potentially present TP tags, lest they get in our way while
    # tesselating with our own method and parameter
    tgui_remtag

    # copy selected objects to internal buffer
    set ay_error ""
    tguiCmd in
    if { $ay_error > 1 } {
	# no objects copied => nothing to tesselate => bail out
	undo rewind; focus .; destroy .tguiw;
	return;
    }

    set f [frame $w.f2]
    button $f.bok -text "Ok" -width 5 -command {
	tguiCmd ok; focus .; destroy .tguiw;
	if { $::ay(need_undo_clear) } { undo clear }
	set ay(ul) $ay(CurrentLevel); uS 0 1; plb_update;
    }
    # button

    button $f.bsa -text "Save" -width 5 -command {
	tguiCmd ca; focus .; destroy .tguiw;
	undo rewind;
	tgui_addtag;
	uCL cl {1 1}; plb_update;
    }
    # button

    button $f.bca -text "Cancel" -width 5 -command {
	global tgui_tessparam
	tguiCmd ca; focus .; destroy .tguiw;
	if { $tgui_tessparam(ReadTag) == 1 } {
	    undo;
	} else {
	    undo rewind;
	}
	uCL cl {1 1}; plb_update;
    }
    # button

    pack $f.bok $f.bsa $f.bca -in $f -side left -fill x -expand yes
    pack $f -in $w -side bottom -fill x

    # Esc-key && close via window decoration == Cancel button
    bind $w <Escape> "$f.bca invoke"
    wm protocol $w WM_DELETE_WINDOW "$f.bca invoke"

    winRestoreOrCenter $w $t
    focus $w.f2.bok

    # create first tesselation
    set tgui_tessparam(OldSamplingMethod) -1
    tgui_update

    # lock tgui_update from events generated by the next sections
    set tgui_tessparam(updatelock) 1

    # initiate update machinery
    event add <<CommitTG>> <KeyPress-Return> <FocusOut>
    catch {event add <<CommitTG>> <Key-KP_Enter>}

    set f $w.f1.fSamplingParamU
    $f.s conf -variable tgui_tessparam(SamplingParamU) -command tgui_update
    #trace variable tgui_tessparam(SamplingParamU) w tgui_update
    bind $f.e <<CommitTG>> "if { !\[string is double \[$f.e get\]\] } break;\
                    if { \[$f.e get\] != \$tgui_tessparam(SamplingParamU) } {\
	                 $f.s conf -command \"\"; \
			 tgui_recalcslider 0 \[$f.e get\]; \
			 $f.s set \[$f.e get\]; \
			 set tgui_tessparam(SamplingParamU) \[$f.e get\]; \
			 $f.s conf -command tgui_update; \
		       };"
    if {$::tcl_platform(platform) == "windows" } {
	bind $f.e <<CommitTG>> "+if { \"%K\" == \"Return\" } {break};"
	bind $f.e <<CommitTG>> "+if { \"%K\" == \"KP_Enter\" } {break};"
    }

    set f $w.f1.fSamplingParamV
    $f.s conf -variable tgui_tessparam(SamplingParamV) -command tgui_update
    #trace variable tgui_tessparam(SamplingParamV) w tgui_update
    bind $f.e <<CommitTG>> "if { !\[string is double \[$f.e get\]\] } break;\
                    if { \[$f.e get\] != \$tgui_tessparam(SamplingParamV) } {\
	                 $f.s conf -command \"\"; \
			 tgui_recalcslider 1 \[$f.e get\]; \
			 $f.s set \[$f.e get\]; \
			 set tgui_tessparam(SamplingParamV) \[$f.e get\]; \
			 $f.s conf -command tgui_update; \
		       };"
    if {$::tcl_platform(platform) == "windows" } {
	bind $f.e <<CommitTG>> "+if { \"%K\" == \"Return\" } {break};"
	bind $f.e <<CommitTG>> "+if { \"%K\" == \"KP_Enter\" } {break};"
    }

    update idletasks

    tgui_establishtraces

    set tgui_tessparam(updatelock) 0

    # establish "Help"-binding
    shortcut_addcshelp $w ayam-5.html tesst

    tgui_block "User interaction is restricted by tesselation dialog!"

    tkwait window $w

    tgui_deletetraces

    after idle tgui_unblock

    after idle viewMouseToCurrent

    winAutoFocusOn

 return;
}
# tgui_open
