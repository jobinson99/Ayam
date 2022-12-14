# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2005 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# User Interface Elements (UIE) for Property GUIs, Preferences, Tool Dialogs,
# and Import/Export Dialogs

# uie_callhelp:
#  call context sensitive help of toplevel window
#  for the provided window
proc uie_callhelp { w } {
    global aymainshortcuts
    set w [winfo toplevel $w]
    set b ""
    catch {set b [bind $w <[repctrl $aymainshortcuts(Help)]>]}
    # use while to protect from 'called break outside of a loop'-error
    while { 1 } {
	if { $b != "" } {
	    eval $b
	}
	break
    }
 return;
}
# uie_callhelp

# uie_fixEntry:
#  helper for entry field based elements, fix tab bindings,
#  add help binding
proc uie_fixEntry { w } {
    global tcl_version tcl_platform aymainshortcuts

    if { ( $tcl_platform(platform) == "windows" ) && \
	 ( $tcl_version > 8.2 ) } {
	bind $w <Tab> {focus [tk_focusNext %W];plb_focus;break}
	bind $w <Shift-Tab> {focus [tk_focusPrev %W];plb_focus;break}
    }

    bind $w <[repctrl $aymainshortcuts(Help)]> "uie_callhelp %W"

 return;
}
# uie_fixEntry

# uie_getEscCmd:
#  helper to get correct escape command, also checks and fixes
#  the current ok button for <Enter>/<Return>
proc uie_getEscCmd { w } {
    global ay

    if { [winfo toplevel $w] == "." } {
	if { ! [winfo exists $ay(bok)] } {
	    set ay(bok) $ay(appb)
	}
	return "resetFocus;break"
    } else {
	return "after idle {$ay(bca) invoke}"
    }
}
# uie_getEscCmd

# uie_setLabelWidth:
#
#
proc uie_setLabelWidth { w width } {
    foreach child [winfo children $w] {
	foreach subwin [winfo children $child] {
	    if { [string last ".l" $subwin] ==
		 [expr [string length $subwin]-2]} {
		$subwin conf -width $width
	    }
	}
    }
 return;
}
# uie_setLabelWidth

# updateParam:
# helper for addParam below;
# realize changes made by the "<"/">" buttons
proc updateParam { w prop name op } {
    global $prop

    set f $w.f${name}

    set oldval [$f.e get]

    if { [string is integer $oldval] || [string is double $oldval] } {

	set newval $oldval
	switch $op {
	    "*2" {
		set newval [expr $oldval * 2]
	    }
	    "/2" {
		set newval [expr $oldval / 2]
	    }
	    "p1" {
		set newval [expr $oldval + 1]
	    }
	    "m1" {
		set newval [expr $oldval - 1]
	    }
	    "p01" {
		set ta $oldval
		set tb 1.0
		set done 0
		if { [expr abs($oldval)] >= 1.0 &&
		     [expr abs($oldval)] <= 10.0 } {
		    if { [string is integer $oldval] } {
			set tb 1
		    } else {
			set tb 0.1
		    }
		    set done 1
		}
		while { ! $done } {
		    if { [expr abs($oldval)] <= 1.0 } {
			if { [expr abs(int($ta) - $ta)] == 0.0 } {
			    set done 1
			} else {
			    set tb [expr $tb/10.0]
			    set ta [expr $ta*10.0]
			}
		    } else {
			if { [expr abs($ta)] < 100.0 } {
			    set done 1
			} else {
			    set tb [expr $tb*10.0]
			    set ta [expr $ta/10.0]
			}
		    }
		}
		if { [string is integer $oldval] } {
		    set newval [expr int($oldval + $tb)]
		} else {
		    set newval [expr $oldval + $tb]
		}
	    }
	    "m01" {
		set ta $oldval
		set tb 1.0
		set done 0
		if { [expr abs($oldval)] >= 1.0 &&
		     [expr abs($oldval)] <= 10.0 } {
		    if { [string is integer $oldval] } {
			set tb 1
		    } else {
			set tb 0.1
		    }
		    set done 1
		}
		while { ! $done } {
		    if { [expr abs($oldval)] <= 1.0 } {
			if { [expr abs(int($ta) - $ta)] == 0.0 } {
			    set done 1
			} else {
			    set tb [expr $tb/10.0]
			    set ta [expr $ta*10.0]
			}
		    } else {
			if { [expr abs($ta)] < 100.0 } {
			    set done 1
			} else {
			    set tb [expr $tb*10.0]
			    set ta [expr $ta/10.0]
			}
		    }
		}
		if { [string is integer $oldval] } {
		    set newval [expr int($oldval - $tb)]
		} else {
		    set newval [expr $oldval - $tb]
		}
	    }
	}
	# switch op
	set ${prop}(${name}) $newval
    }
    # if isinteger || isdouble
 return;
}
# updateParam

# addParam with balloon help
proc addParamB { w prop name help {def {}} } {

    addParam $w $prop $name $def

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addParamB

# addParam:
#  add UIE for manipulation of a single numeric parameter
proc addParam { w prop name {def {}} } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    regsub -all $ay(fixvname) $name "_" fname
    set f [frame $w.f${fname} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:

    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $fname"

    set e [entry $f.e -width 8 -textvariable ${prop}(${fname}) -bd $bw \
	       -highlightthickness $hl]
    eval [subst "bindtags $f.e \{$f.e pge Entry all\}"]
    bind $f.e <Key-Escape> $escapecmd

    uie_fixEntry $f.e
    bind $f.e <Double-ButtonPress-1> "$f.e selection range 0 end; break"

    set font [$e cget -font]
    set wi [expr [font measure $font "WW"]]
    set he [expr [winfo reqheight $e] - 6]
    if { $ay(ws) == "Aqua" } {
	incr he -5
    }
    button $f.b1 -bd $bw -wi $wi -he $he -image ay_TriangleL_img\
	-command "updateParam $w $prop $fname /2"\
	-takefocus 0 -highlightthickness 0

    button $f.b2 -bd $bw -wi $wi -he $he -image ay_TriangleR_img\
	-command "updateParam $w $prop $fname *2"\
	-takefocus 0 -highlightthickness 0

    set mb ""
    if { $def != {} } {
	set mbs [expr [winfo reqheight $f.b2] - 2]
	if { $ay(ws) == "Aqua" } {
	    incr mbs -8
	}
	if { $ay(ws) == "Windows" } {
	    incr mbs 4
	}

	set mb [menubutton $f.b3 -height $mbs -width $mbs -bd $bw\
		    -image ay_Triangle_img -takefocus 0\
		    -highlightthickness 0 -relief raised -menu $f.b3.m]

	if { $ay(ws) == "Aqua" } {
	    $mb conf -height [$f.b2 cget -height]
	}

	set m [menu $mb.m -tearoff 0]
	foreach val $def {
	    $m add command -label $val\
		-command "global $prop; $e delete 0 end; $e insert end $val;"
	}
	bind $f.e <Key-F10> "$f.b3.m post \[winfo rootx $mb\]\
         \[winfo rooty $mb\];tk_menuSetFocus $f.b3.m;break"
    }

    bind $f.b1 <Control-ButtonPress-1> "updateParam $w $prop $fname m01;break"
    bind $f.b2 <Control-ButtonPress-1> "updateParam $w $prop $fname p01;break"

    bind $f.b1 <Alt-ButtonPress-1> "updateParam $w $prop $fname m1;break"
    bind $f.b2 <Alt-ButtonPress-1> "updateParam $w $prop $fname p1;break"

    if { ! $ay(iapplydisable) } {
	global aymainshortcuts
	bind $f.b1 <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
	    "after idle {$ay(bok) invoke}"
	bind $f.b2 <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
	    "after idle {$ay(bok) invoke}"
	if { $mb != "" } {
	    bind $mb <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
		"after idle {$ay(bok) invoke}"
	}
    }

    bind $f.e <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $f.e <Key-KP_Enter> "$::ay(bok) invoke;break"}

    if { $ay(ws) == "Windows" } {
	pack $f.l -in $f -side left
	pack $f.b1 -in $f -side left -pady 1 -fill both -expand no
	pack $f.e -in $f -side left -pady 1 -fill both -expand yes
	pack $f.b2 -in $f -side left -pady 1 -fill both -expand no
	if { $mb != "" } { pack $mb -side left -pady 1 -fill both -expand no}
    } else {
	if { $ay(ws) != "Aqua" } {
	    set hl [expr round($ay(GuiScale))]

	    $f.b1 configure -highlightthickness $hl
	    $f.b2 configure -highlightthickness $hl
	    if { $mb != "" } { $mb configure -highlightthickness $hl }
	}
	pack $f.l -in $f -side left -fill x -expand no
	pack $f.b1 -in $f -side left -fill x -expand no
	pack $f.e -in $f -side left -fill x -expand yes
	pack $f.b2 -in $f -side left -fill x -expand no
	if { $mb != "" } {
	    pack $mb -side left -fill x -expand no;
	    if { $ay(ws) == "Aqua" } {
		pack conf $mb -anchor n
	    }
	}
    }
    pack $f -in $w -side top -fill x

 return;
}
# addParam

# addParamPair:
#  add a pair of Param UIEs with one name
proc addParamPair { w prop name {def {}} } {
    set fdef {}
    set sdef {}
    foreach i $def {
	lappend fdef [lindex $i 0]
	lappend sdef [lindex $i 1]
    }
    addParam $w $prop ${name}_0 $fdef
    addParam $w $prop ${name}_1 $sdef
 return;
}
# addParamPair

# addMatrix with balloon help
proc addMatrixB { w prop name help } {

    addMatrix $w $prop $name

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addMatrixB

# addMatrix:
#  add a UIE for the manipulation of a 4*4 matrix
proc addMatrix { w prop name } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]
    set bw 1

    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l -text ${name}:

    for { set i 0 } { $i < 16 } { incr i } {
	lappend omitl ${name}_$i
    }

    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l { $omitl }"
    pack $f.l -in $f
    set f1 [frame $w.f${name}1 -relief sunken -bd $bw]
    for { set i 0 } { $i < 4 } { incr i } {
	entry $f1.e$i -width 6 -textvariable ${prop}(${name}_${i}) -bd $bw
	eval [subst "bindtags $f1.e$i \{$f1.e$i pge Entry all\}"]
	bind $f1.e$i <Key-Escape> $escapecmd
	uie_fixEntry $f1.e$i
	pack $f1.e$i -in $f1 -side left -fill both -expand yes
    }
    set f2 [frame $w.f${name}2 -relief sunken -bd $bw]
    for { set i 4 } { $i < 8 } { incr i } {
	entry $f2.e$i -width 6 -textvariable ${prop}(${name}_${i}) -bd $bw
	eval [subst "bindtags $f2.e$i \{$f2.e$i pge Entry all\}"]
	bind $f2.e$i <Key-Escape> $escapecmd
	uie_fixEntry $f2.e$i
	pack $f2.e$i -in $f2 -side left -fill both -expand yes
    }
    set f3 [frame $w.f${name}3 -relief sunken -bd $bw]
    for { set i 8 } { $i < 12 } { incr i } {
	entry $f3.e$i -width 6 -textvariable ${prop}(${name}_${i}) -bd $bw
	eval [subst "bindtags $f3.e$i \{$f3.e$i pge Entry all\}"]
	bind $f3.e$i <Key-Escape> $escapecmd
	uie_fixEntry $f3.e$i
	pack $f3.e$i -in $f3 -side left -fill both -expand yes
    }
    set f4 [frame $w.f${name}4 -relief sunken -bd $bw]
    for { set i 12 } { $i < 16 } { incr i } {
	entry $f4.e$i -width 6 -textvariable ${prop}(${name}_${i}) -bd $bw
	eval [subst "bindtags $f4.e$i \{$f4.e$i pge Entry all\}"]
	bind $f4.e$i <Key-Escape> $escapecmd
	uie_fixEntry $f4.e$i
	pack $f4.e$i -in $f4 -side left -fill both -expand yes
    }

    pack $f $f1 $f2 $f3 $f4 -in $w -side top -fill x -expand yes

 return;
}
# addMatrix

#chooseColor:
# helper for addColor below;
# open a color chooser dialog
proc chooseColor { w prop name button } {
    global $prop

    winAutoFocusOff

    set rname ${prop}(${name}_R)
    set gname ${prop}(${name}_G)
    set bname ${prop}(${name}_B)

    set newcolor [tk_chooseColor -parent [winfo toplevel $w]\
		      -initialcolor [$button cget -background]]
    if { $newcolor != "" } {
	$button configure -background $newcolor

	set label [string replace $button end-1 end l2]
	if { [winfo exists $label] } {
	    $label configure -background $newcolor
	}

	scan $newcolor "#%2x%2x%2x" r g b
	set $rname $r
	set $gname $g
	set $bname $b
    }

    winAutoFocusOn

 return;
}
# chooseColor

# updateColorFromE:
# helper for addColor below;
# realize changes made in the color value entries
proc updateColorFromE { w prop name button } {
    global $prop

    set rname ${prop}(${name}_R)
    set gname ${prop}(${name}_G)
    set bname ${prop}(${name}_B)

    set red [subst "\$$rname"]
    set green [subst "\$$gname"]
    set blue [subst "\$$bname"]

    # clamp colorvalues to correct range
    if { $red < -1 } { set $rname 0; set red 0 }
    if { $red > 255 } { set $rname 255; set red 255 }

    if { $green < -1 } { set $gname 0; set green 0 }
    if { $green > 255 } { set $gname 255; set green 255 }

    if { $blue < -1 } { set $bname 0; set blue 0 }
    if { $blue > 255 } { set $bname 255; set blue 255 }
    if { ($red == -1) || ($green == -1) || ($blue == -1) } {
	set newcolor black
    } else {
	set newcolor [format "#%02x%02x%02x" $red $green $blue]
    }
    $button configure -background $newcolor

    set label [string replace $button end-1 end l2]
    if { [winfo exists $label] } {
	$label configure -background $newcolor
    }

 return;
}
# proc updateColorFromE

# addColor with balloon help
proc addColorB { w prop name help {def {}} } {

    addColor $w $prop $name $def

    balloon_set $w.fl${name}.l "${name}:\n${help}"

 return;
}
# addColorB

# addColor:
#  add UIE for a color parameter
proc addColor { w prop name {def {}} } {
    global $prop ay ayprefs aymainshortcuts

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    # first, we create a label on its own line
    set f [frame $w.fl${name} -relief sunken -bd $bw]

    label $f.l -text $name
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l {${name}_R\
	    ${name}_G ${name}_B }"
    pack $f.l -in $f -side top
    pack $f -in $w -side top -fill x -expand yes

    # now the color entries
    set f [frame $w.f${name}]

    set e1 [entry $f.er -width 4 -textvariable ${prop}(${name}_R) -bd $bw \
	       -highlightthickness $hl]
    eval [subst "bindtags $f.er \{$f.er pge Entry all\}"]
    bind $e1 <Key-Escape> $escapecmd
    uie_fixEntry $e1
    set e2 [entry $f.eg -width 4 -textvariable ${prop}(${name}_G) -bd $bw \
		-highlightthickness $hl]
    eval [subst "bindtags $f.eg \{$f.eg pge Entry all\}"]
    bind $e2 <Key-Escape> $escapecmd
    uie_fixEntry $e2
    set e3 [entry $f.eb -width 4 -textvariable ${prop}(${name}_B) -bd $bw \
	       -highlightthickness $hl]
    eval [subst "bindtags $f.eb \{$f.eb pge Entry all\}"]
    bind $e3 <Key-Escape> $escapecmd
    uie_fixEntry $e3

    bind $e1 <FocusOut> "updateColorFromE $w $prop $name $f.b1"
    bind $e2 <FocusOut> "updateColorFromE $w $prop $name $f.b1"
    bind $e3 <FocusOut> "updateColorFromE $w $prop $name $f.b1"

    set red [subst \$${prop}(${name}_R)]
    set green [subst \$${prop}(${name}_G)]
    set blue [subst \$${prop}(${name}_B)]

    if { ($red <= 255) && ($red >= 0) && ($green <= 255) && ($green >= 0)\
	    && ($blue <= 255) && ($blue >= 0)} {
	set bcolor [format "#%02x%02x%02x" $red $green $blue]
    } else {
	set bcolor "#000000"
    }

    if { $ay(ws) == "Windows" } {
	button $f.b1 -pady 1 -background $bcolor\
		-command "chooseColor $w $prop $name $f.b1"\
		-bd $bw -width 3 -highlightthickness $hl
    } else {
	if { $ay(ws) == "Aqua" } {
	    label $f.l2 -background $bcolor -text "   "
	    button $f.b1 -pady 1 -padx 6 -text Set\
		-command "chooseColor $w $prop $name $f.b1"\
		-bd $bw -highlightthickness $hl
	} else {
	    # X11
	    button $f.b1 -pady 1 -background $bcolor\
		-command "chooseColor $w $prop $name $f.b1"\
		-bd $bw -highlightthickness $hl
	}
    }
    # if

    eval [subst "bindtags $f.b1 \{$f.b1 pge Button all\}"]
    bind $f.b1 <Key-Escape> $escapecmd
    bind $f.b1 [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"
    bind $f.b1 <Visibility> "updateColorFromE $w $prop $name $f.b1"

    set mb ""
    if { $def != {} } {
	set mbs [expr [winfo reqheight $f.b1] - 2]
	set mb [menubutton $f.b3 -height $mbs -width $mbs -bd $bw\
		    -image ay_Triangle_img -takefocus 0\
		    -highlightthickness 0 -relief raised -menu $f.b3.m]
	if { $ay(ws) == "Windows" } {
	    $mb configure -pady 0
	}
	if { $ay(ws) == "Aqua" } {
	    $mb configure -height 14
	}
	set m [menu $mb.m -tearoff 0]
	foreach val $def {
	    $m add command -label "$val"\
	     -command "global $prop;\
	     $e1 delete 0 end;\
	     $e1 insert end [lindex $val 0];\
	     $e2 delete 0 end;\
	     $e2 insert end [lindex $val 1];\
	     $e3 delete 0 end;\
	     $e3 insert end [lindex $val 2];\
	     updateColorFromE $w $prop $name $f.b1"
	}
	# foreach
	bind $f <Key-F10> "$f.b3.m post \[winfo rootx $mb\]\
         \[winfo rooty $mb\];tk_menuSetFocus $f.b3.m;break"
    }
    # if

    if { ! $ay(iapplydisable) } {
	global aymainshortcuts
	if { $mb != "" } {
	    bind $mb <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
		"after idle {$ay(bok) invoke}"
	}
	bind $f.b1 <${aymainshortcuts(IApplyMod)}-ButtonPress-1>\
		"chooseColor $w $prop $name $f.b1;\
		 after idle {$ay(bok) invoke};break;"
    }

    set okcmd "updateColorFromE $w $prop $name $f.b1;$ay(bok) invoke;break"
    bind $e1 <Key-Return> $okcmd
    catch {bind $e1 <Key-KP_Enter> $okcmd}
    bind $e2 <Key-Return> $okcmd
    catch {bind $e2 <Key-KP_Enter> $okcmd}
    bind $e3 <Key-Return> $okcmd
    catch {bind $e3 <Key-KP_Enter> $okcmd}

    if { [winfo exists $f.l2] } {
	pack $f.er $f.eg $f.eb $f.l2 $f.b1 -in $f -fill both -expand yes\
		-side left -padx 2
    } else {
	pack $f.er $f.eg $f.eb $f.b1 -in $f -fill both -expand yes\
		-side left -padx 2
    }

    if { $mb != "" } { pack $mb -side left -fill both -expand yes}

    pack $f -in $w.fl${name} -side bottom -fill x

 return;
}
# addColor

# addCheck with balloon help
proc addCheckB { w prop name help {onoffvals ""} } {

    addCheck $w $prop $name $onoffvals

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addCheckB

# addCheck:
#  add UIE for a boolean parameter
proc addCheck { w prop name {onoffvals ""} } {
    global $prop ay ayprefs aymainshortcuts tcl_version

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    regsub -all $ay(fixvname) $name "_" fname
    set f [frame $w.f${fname} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $fname"

    if { $ay(ws) == "Windows" ||
	 ($ay(ws) != "Aqua" && $tcl_version > 8.4) } {
	# damn windows
	set ff [frame $f.fcb -highlightthickness $hl]
	set cb [checkbutton $ff.cb -image ay_Empty66_img\
		    -variable ${prop}(${fname})\
		    -bd $bw -indicatoron 0 -selectcolor #b03060]

	bind $ff <Enter> { %W configure -background #ececec }
	if { $ay(ws) == "Windows" } {
	    bind $ff <Leave> { %W configure -background SystemButtonFace }
	} else {
	    set bgc [$ff cget -background]
	    bind $ff <Leave> "%W configure -background $bgc"
	}
	bind $ff <1> { %W.cb invoke }

	pack $f.l -in $f -side left
	pack $f.fcb -in $f -side left -expand yes -fill both
	pack $ff.cb -in $ff -side top -padx 30\
	    -pady [expr 3 * round($ay(GuiScale))]

	eval [subst "bindtags $ff.cb \{$ff.cb pge Checkbutton all\}"]
	bind $ff.cb <Key-Escape> $escapecmd
	bind $ff.cb [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"
    } else {
	if { $ay(ws) == "Aqua" } {
	    # also Aqua gets its "Extrawurst"
	    set ff [frame $f.fcb -highlightthickness $hl]
	    set cb [checkbutton $ff.cb -variable ${prop}(${fname}) -bd $bw]
	    bind $ff <1> { %W.cb invoke }
	    pack $f.l -in $f -side left
	    pack $f.fcb -in $f -side left -expand yes -fill both
	    pack $ff.cb -in $ff -side top -padx 30 -pady 0

	    eval [subst "bindtags $ff.cb \{$ff.cb pge Checkbutton all\}"]
	    bind $ff.cb <Key-Escape> $escapecmd
	    bind $ff.cb [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"
	} else {
	    # generic (X11) implementation
	    set cb [checkbutton $f.cb -variable ${prop}(${fname}) -bd $bw\
		    -pady 1 -padx 30 -highlightthickness $hl]
	    pack $f.l -in $f -side left
	    pack $f.cb -in $f -side left -fill x -expand yes

	    eval [subst "bindtags $f.cb \{$f.cb pge Checkbutton all\}"]
	    bind $f.cb <Key-Escape> $escapecmd
	    bind $f.cb [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"
	}
    }

    if { ! $ay(iapplydisable) } {
	global aymainshortcuts
	bind $cb <${aymainshortcuts(IApplyMod)}-ButtonRelease-1> "after idle {\
	    $ay(bok) invoke}"

	if { $ay(ws) == "Windows" || $ay(ws) == "Aqua" || $tcl_version > 8.4 } {
	    bind $ff <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
		"after idle {$ay(bok) invoke}"
	}
    }

    bind $cb <Key-Return> "$ay(bok) invoke;break"
    catch {bind $cb <Key-KP_Enter> "$ay(bok) invoke;break"}

    if { $onoffvals != "" } {
	$cb conf -onvalue [lindex $onoffvals 0]
	$cb conf -offvalue [lindex $onoffvals 1]
    }

    pack $f -in $w -side top -fill x

 return;
}
# addCheck

# updateMenu:
# helper for addMenu below;
# write trace procedure
proc updateMenu { m name1 name2 op } {
    global ${name1}
    $m invoke [subst \$${name1}($name2)]
 return;
}
# updateMenu

# addMenu with balloon help
proc addMenuB { w prop name help elist } {

    addMenu $w $prop $name $elist

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addMenuB

# addMenu:
#  add UIE for a choice parameter realized as drop-down menu
proc addMenu { w prop name elist } {
    global $prop ay ayprefs aymainshortcuts

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    regsub -all $ay(fixvname) $name "_" fname
    set f [frame $w.f${fname} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $fname"

    set l 0
    foreach i $elist {
	if { [string length $i] > $l } {
	    set l [string length $i]
	}
    }
    incr l

    menubutton $f.mb -text Eimer -menu $f.mb.m -relief raised -bd $bw\
	-padx 0 -pady 1 -takefocus 1 -width $l -highlightthickness $hl\
	-indicatoron 1

    eval [subst "bindtags $f.mb \{$f.mb pge Menubutton all\}"]
    bind $f.mb <Key-Escape> $escapecmd
    bind $f.mb <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $f.mb <Key-KP_Enter> "$::ay(bok) invoke;break"}

    bind $f.mb [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"

    if { $ay(ws) == "Windows" } {
	$f.mb configure -pady 1 -highlightthickness 0
	bind $f.mb <FocusIn> "$f.mb conf -highlightthickness $hl"
	bind $f.mb <FocusOut> "$f.mb conf -highlightthickness 0"
    }

    if { $ay(ws) == "Aqua" } {
	$f.mb configure -pady 2 -width $l
    }

    set m [menu $f.mb.m -tearoff 0]

    set val 0

    foreach i $elist {
	$m add command -label $i -command "global ${prop};\
		catch {set ${prop}($fname) $val};\
		$f.mb configure -text {$i}"
	incr val
    }

    $m invoke [subst \$${prop}($fname)]

    trace variable ${prop}($fname) w "updateMenu $m"

    bind $f.mb <Destroy> "trace vdelete ${prop}($fname) w \"updateMenu $m\""

    pack $f.l -in $f -side left -fill x
    pack $f.mb -in $f -side left -fill x -expand yes -pady 0
    pack $f -in $w -side top -fill x

    if { ! $ay(iapplydisable) } {
	global aymainshortcuts
	bind $f.mb <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
	    "after idle {$ay(bok) invoke}"
    }

 return;
}
# addMenu

# addString with balloon help
proc addStringB { w prop name help {def {}} } {

    addString $w $prop $name $def

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addStringB

# addString:
#  add UIE for a string parameter
proc addString { w prop name {def {}} } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]

    if { $ay(ws) == "Windows" } {
	append escapecmd ";break"
    }

    set bw 1
    set hl [expr round($ay(GuiScale))]

    regsub -all $ay(fixvname) $name "_" fname
    set f [frame $w.f${fname} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $fname"

    set e [entry $f.e -textvariable ${prop}(${fname}) -width 15 -bd $bw \
	      -highlightthickness $hl]
    eval [subst "bindtags $f.e \{$f.e pge Entry all\}"]
    bind $f.e <Key-Escape> $escapecmd
    uie_fixEntry $f.e

    bind $f.e <Key-Return> "$ay(bok) invoke;break"
    catch {bind $f.e <Key-KP_Enter> "$ay(bok) invoke;break"}

    set mb ""
    if { $def != {} } {
	set mbs [expr [winfo reqheight $f.e] - 2]
	set mb [menubutton $f.b3 -height $mbs -width $mbs -bd $bw\
		    -image ay_Triangle_img -takefocus 0\
		-highlightthickness 0 -relief raised -menu $f.b3.m]

	if { $ay(ws) == "Windows" } {
	    $mb configure -pady 0
	}

	if { $ay(ws) == "Aqua" } {
	    $mb configure -height 14
	}

	set m [menu $mb.m -tearoff 0]
	foreach val $def {
	    if { $val == "..." } {
		$m add command -label $val\
	    -command "global $prop; $e delete 0 end; focus -force $e"
	    } else {
		$m add command -label $val\
	      -command "global $prop; $e delete 0 end; $e insert end \{$val\};"
	    }
	}

	if { ! $ay(iapplydisable) } {
	    global aymainshortcuts
	    bind $mb <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
		"after idle {$ay(bok) invoke}"
	}
	bind $f.e <Key-F10> "$f.b3.m post \[winfo rootx $mb\]\
         \[winfo rooty $mb\];tk_menuSetFocus $f.b3.m;break"
    }

    pack $f.l -in $f -side left -fill x
    pack $f.e -in $f -side left -fill both -expand yes
    if { $mb != "" } { pack $mb -side left -fill both -expand no}

    pack $f -in $w -side top -fill x

 return;
}
# addString

# entryViewEnd:
# helper for addFile* below;
# scrolls the entry view to the end after a (long) path/filename has been set
proc entryViewEnd { w } {
    $w icursor end;
    set c [$w index insert]
    set left [$w index @0]
    if {$left > $c} {
	$w xview $c
	return;
    }
    set x [winfo width $w]
    if {$c > [$w index @[winfo width $w]]} {
	$w xview insert
    }
 return;
}
# entryViewEnd

# addFileT with balloon help
proc addFileTB { w prop name ftypes help {def {}} } {

    addFileT $w $prop $name $ftypes $def

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addFileTB

# addFileT:
#  add UIE for a file name with specified file type
proc addFileT { w prop name ftypes {def {}} } {

    addFile $w $prop $name $def
    set f $w.f${name}
    $f.b configure -command "
	    global $prop ay;
            winAutoFocusOff;
	    set filen \[$f.e get\];
            if { \$ay(ws) != \"Aqua\" } {
	      set filen \[tk_getOpenFile -filetypes {$ftypes}\
                          -parent [winfo toplevel $w]\
	                  -title \"Set File:\"];
            } else {
	      set filen \[tk_getOpenFile -filetypes {$ftypes}\
	                  -title \"Set File:\"];
            };
            winAutoFocusOn;
	    if { \$filen != \"\" } {
		$f.e delete 0 end;
		$f.e insert 0 \$filen;
		entryViewEnd $f.e;
		set ${prop}($name) \$filen;
	    }"

 return;
}
# addFileT

# addSFileT:
#  add UIE for a file name for saving with specified file type
proc addSFileT { w prop name ftypes {def {}} } {

    addFile $w $prop $name $def
    set f $w.f${name}
    $f.b configure -command "\
	    global $prop ay;
            winAutoFocusOff;
	    set filen \[$f.e get\];
            if { \$ay(ws) != \"Aqua\" } {
              set filen \[tk_getSaveFile -filetypes {$ftypes}\
                          -parent [winfo toplevel $w]\
		          -title \"Set File:\"];
            } else {
              set filen \[tk_getSaveFile -filetypes {$ftypes}\
		          -title \"Set File:\"];
            };
            winAutoFocusOn;
	    if { \$filen != \"\" } {
		$f.e delete 0 end;
		$f.e insert 0 \$filen;
		entryViewEnd $f.e;
		set ${prop}($name) \$filen;
	    }"

 return;
}
# addSFileT

# addSFile:
#  add UIE for a file name for saving
proc addSFile { w prop name {def {}} } {

    addFile $w $prop $name $def
    set f $w.f${name}
    $f.b configure -command "\
	    global $prop ay;
            winAutoFocusOff;
	    set filen \[$f.e get\];
            if { \$ay(ws) != \"Aqua\" } {
              set filen \[tk_getSaveFile -parent [winfo toplevel $w]\
		          -title \"Set File:\"];
            } else {
              set filen \[tk_getSaveFile -title \"Set File:\"];
            };
            winAutoFocusOn;
	    if { \$filen != \"\" } {
		$f.e delete 0 end;
		$f.e insert 0 \$filen;
		entryViewEnd $f.e;
		set ${prop}($name) \$filen;
	    }"

 return;
}
# addSFile

# addSFile with balloon help
proc addSFileB { w prop name help {def {}} } {

    addSFile $w $prop $name $def

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addSFileB

# addFile with balloon help
proc addFileB { w prop name help {def {}} } {

    addFile $w $prop $name $def

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addFileB

# addFile:
#  add UIE for a file name
proc addFile { w prop name {def {}} } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $name"

    set e [entry $f.e -textvariable ${prop}(${name}) -width 15 -bd $bw \
	      -highlightthickness $hl]
    eval [subst "bindtags $f.e \{$f.e pge Entry all\}"]
    bind $f.e <Key-Escape> $escapecmd
    uie_fixEntry $f.e
    after idle "entryViewEnd $f.e"

    bind $f.e <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $f.e <Key-KP_Enter> "$::ay(bok) invoke;break"}

    button $f.b -text "Set" -width 4 -bd $bw -padx 0 -pady 0 -takefocus 0\
     -command "\
	global $prop ay;
        winAutoFocusOff;
	set filen \[$f.e get\];
        if { \$ay(ws) != \"Aqua\" } {
  	  set filen \[tk_getOpenFile -parent [winfo toplevel $w]\
                       -title \"Set File:\"];
        } else {
  	  set filen \[tk_getOpenFile -title \"Set File:\"];
        };
        winAutoFocusOn;
	if { \$filen != \"\" } {
	    $f.e delete 0 end;
	    $f.e insert 0 \$filen;
	    entryViewEnd $f.e;
	    set ${prop}($name) \$filen;
        }"

    if { $ay(ws) == "Aqua" } {
	$f.b configure -padx 6
    }

    set mb ""
    if { $def != {} } {
	set mbs [expr [winfo reqheight $f.b] - 2]
	set mb [menubutton $f.b3 -height $mbs -width $mbs -bd $bw\
		    -image ay_Triangle_img -takefocus 0\
		    -highlightthickness 0 -relief raised -menu $f.b3.m]
	if { $ay(ws) == "Windows" } {
	    $mb configure -pady 0
	}
	if { $ay(ws) == "Aqua" } {
	    $f.b3 configure -pady 2
	    $mb configure -height 14
	}
	set m [menu $mb.m -tearoff 0]
	foreach val $def {
	    $m add command -label $val\
		    -command "global $prop; $e delete 0 end;
	                      $e insert end \{$val\};"
	}
	# foreach
	bind $f.e <Key-F10> "$f.b3.m post \[winfo rootx $mb\]\
         \[winfo rooty $mb\];tk_menuSetFocus $f.b3.m;break"
    }
    # if

    pack $f.l -in $f -side left -fill x -expand no
    pack $f.e -in $f -side left -fill both -expand yes
    pack $f.b -in $f -side left -fill x -expand no

    if { $mb != "" } {
	if { $ay(ws) == "Windows" } {
	    pack $mb -side right -fill both -expand no
	} else {
	    pack $mb -side right -fill x -expand no
	}
    }

    pack $f -in $w -side top -fill x

 return;
}
# addFile

# addMDir with balloon help
proc addMDirB { w prop name help } {

    addMDir $w $prop $name

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addMDirB

# addMDir:
#  add UIE for a list of directory names
proc addMDir { w prop name } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $name"

    entry $f.e -textvariable ${prop}(${name}) -width 15 -bd $bw \
	-highlightthickness $hl
    eval [subst "bindtags $f.e \{$f.e pge Entry all\}"]
    bind $f.e <Key-Escape> $escapecmd
    uie_fixEntry $f.e

    bind $f.e <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $f.e <Key-KP_Enter> "$::ay(bok) invoke;break"}

    bind $f.e <Key> "+destroy $f.e.balloon;"

    bind $f.e <1> "+after 500 \{balloon_setsplit $f.e \[$f.e get\] 15\}"
    eval balloon_setsplit $f.e \$${prop}(${name}) 15

    button $f.b -text "Add" -width 4 -bd $bw -padx 0 -pady 0 -takefocus 0\
     -command "\
	global $prop ay;
        winAutoFocusOff;
        if { \$ay(ws) != \"Aqua\" } {
	  set dir \[tk_chooseDirectory -parent [winfo toplevel $w]\
                     -initialdir \[pwd\]\];
        } else {
	  set dir \[tk_chooseDirectory\];
        };
        winAutoFocusOn;
	if { \${dir} != \"\" } {
	  if { \$${prop}($name) != \"\" } {
	      set sep \$ay(separator);
	      set ${prop}($name) \$${prop}($name)\${sep}\${dir};
	  } else {
	      set ${prop}($name) \${dir};
	  };
	  entryViewEnd $f.e;
	  update;
	  eval balloon_setsplit $f.e \[list \$${prop}($name)\] 15;
	};"

    if { $ay(ws) == "Aqua" } {
	$f.b configure -padx 6
    }

    pack $f.l -in $f -side left -fill x -expand no
    pack $f.e -in $f -side left -fill both -expand yes
    pack $f.b -in $f -side left -fill x -expand no
    pack $f -in $w -side top -fill x

 return;
}
# addMDir

# addMFile with balloon help
proc addMFileB { w prop name help } {

    addMFile $w $prop $name

    balloon_set $w.f${name}.l "${name}:\n${help}"

 return;
}
# addMFileB

# addMFile:
#  add a UIE for a list of file names
proc addMFile { w prop name } {
    global $prop ay ayprefs

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l -width 14 -text ${name}:
    bind $f.l <Double-ButtonPress-1> "pclip_toggleomit $f.l $name"

    entry $f.e -textvariable ${prop}(${name}) -width 15 -bd $bw \
	-highlightthickness $hl
    eval [subst "bindtags $f.e \{$f.e pge Entry all\}"]
    bind $f.e <Key-Escape> $escapecmd
    uie_fixEntry $f.e

    bind $f.e <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $f.e <Key-KP_Enter> "$::ay(bok) invoke;break"}

    bind $f.e <Key> "+destroy $f.e.balloon;"

    bind $f.e <1> "+after 500 \{balloon_setsplit $f.e \[$f.e get\] 15\}"
    eval balloon_setsplit $f.e \$${prop}(${name}) 15

    button $f.b -text "Add" -width 4 -bd $bw -padx 0 -pady 0 -takefocus 0\
     -command "\
	global $prop ay;
        winAutoFocusOff;
	set filen \[$f.e get\];
	global ay;
	set sep \$ay(separator);
        if { \$ay(ws) != \"Aqua\" } {
	  set filen \[tk_getOpenFile -parent [winfo toplevel $w]\
                     -title \"Select File:\"\];
        } else {
	  set filen \[tk_getOpenFile -title \"Select File:\"\];
        };
        winAutoFocusOn;
	if { \$filen != \"\" } {
	 if { \$${prop}($name) != \"\" } {
          set ${prop}($name) \$${prop}($name)\$sep\$filen;
	 } else {
	  set ${prop}($name) \$filen;
         };
	 entryViewEnd $f.e;
	 update;
         eval balloon_setsplit $f.e \[list \$${prop}($name)\] 15;
        };"

    if { $ay(ws) == "Aqua" } {
	$f.b configure -padx 6
    }

    pack $f.l -in $f -side left -fill x
    pack $f.e -in $f -side left -fill both -expand yes
    pack $f.b -in $f -side left -fill x
    pack $f -in $w -side top -fill x

 return;
}
# addMFile

# addCommand with balloon help
proc addCommandB { w name text command help } {

    addCommand $w $name $text $command

    balloon_set $w.f${name}.b "${text}:\n${help}"

 return;
}
# addCommandB

# addCommand:
#  add UIE to let the user start an action
proc addCommand { w name text command } {
    global ay ayprefs aymainshortcuts

    set escapecmd [uie_getEscCmd $w]
    set bw 1
    set hl [expr round($ay(GuiScale))]

    set f [frame $w.f${name} -relief sunken -bd $bw]

    button $f.b -text $text -bd $bw -command $command -pady 0 \
	-highlightthickness $hl
    eval [subst "bindtags $f.b \{$f.b pge Button all\}"]
    bind $f.b <Key-Escape> $escapecmd
    bind $f.b [repctrl $aymainshortcuts(Help)] "uie_callhelp %W"

    if { ! $ay(iapplydisable) } {
	global aymainshortcuts
	bind $f.b <${aymainshortcuts(IApplyMod)}-ButtonRelease-1>\
	    "global ay; set ay(shiftcommand) 1;"
    }

    pack $f.b -in $f -side left -fill x -expand yes
    pack $f -in $w -side top -fill x

 return;
}
# addCommand

# addText:
#  add UIE to display static text (e.g. group/section names)
proc addText { w name text } {

    set f [frame $w.f${name}]

    label $f.tl -text $text

    pack $f.tl -in $f -side left -fill x -expand yes
    pack $f -in $w -side top

 return;
}
# addText

# addInfo with balloon help
proc addInfoB { w prop name help } {

    addInfo $w $prop $name

    balloon_set $w.f${name}.l1 "${name}:\n${help}"

 return;
}
# addInfoB

# updateInfoBalloon:
# helper for addInfo below;
# write trace procedure
proc updateInfoBalloon { f name1 name2 op } {
    global ${name1}
    set newtext [subst \$${name1}($name2)]
    balloon_set $f.l2 "$newtext"
 return;
}
# updateInfoBalloon

# addInfo:
#  add UIE to convey dynamic textual information with a label
#
proc addInfo { w prop name } {
    set bw 1
    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l1 -width 14 -text ${name}:

    entry $f.e
    label $f.l2 -textvariable ${prop}(${name}) -font [$f.e cget -font]

    destroy $f.e

    pack $f.l1 -in $f -side left -fill x -expand no
    pack $f.l2 -in $f -side left -fill x -expand yes
    pack $f -in $w -side top -fill x
    global $prop
    if { [info exists "${prop}(${name}Ball)"] } {
	global $prop
	set balltext [subst \$${prop}(${name}Ball)]
	balloon_set $f.l2 "$balltext"
	trace variable ${prop}(${name}Ball) w "updateInfoBalloon $f"
    }
 return;
}
# addInfo

# updateProgress:
# helper for addProgress below;
# write trace procedure
proc updateProgress { w n1 n2 op } {
    if { $n2 != "" } {
	global $n1
	eval "SetProgress $w \$${n1}(${n2})"
    }
 return;
}
# updateProgress

# addProgress:
#  add UIE to convey arbitrary operation progress
proc addProgress { w prop name } {
    global $prop

    set bw 1
    set f [frame $w.f${name} -relief sunken -bd $bw]

    label $f.l1 -width 14 -text ${name}:

    Progress $f.p

    set tracecommand "updateProgress $f.p"

    trace variable ${prop}(${name}) w $tracecommand

    bind $f.p <Destroy> "trace vdelete ${prop}(${name}) w {$tracecommand}"

    pack $f.l1 -in $f -side left -fill x -expand no
    pack $f.p -in $f -side left -fill x -expand yes
    pack $f -in $w -side top -fill x

 return;
}
# addProgress

# addVSpace
#  add vertical space element to improve property GUI layouts
#  that do not start with labels
proc addVSpace { w name h } {
    set f [frame $w.f${name}]
    set l [label $w.f${name}.${name} -height $h -image ay_Empty_img]
    pack $l -in $f
    pack $f -in $w -side top -fill x
 return;
}
# addVSpace

# addPropertyGUI:
#  create frame and global property management array
proc addPropertyGUI { name {sproc ""} {gproc ""} } {
    global ay $name

    set w $ay(pca).f$name
    set ay(bok) $ay(appb)

    if { ![info exists $name] } {
	frame $w
	set ${name}(arr) ${name}Data
	set ${name}(sproc) $sproc
	set ${name}(gproc) $gproc
	set ${name}(w) f$name

	set ::${name}GUI 1
    }

 return $w;
}
# addPropertyGUI

# updateOptionToggle:
# helper for addOptionToggle below;
# toggles the button image and calls the user specified toggling command
proc updateOptionToggle { w prop name cmd } {
    global $prop
    set b $w.f${name}.b
    eval "set v \$${prop}($name)"
    if { $v == 1 } {
	$b conf -image ay_TriangleR_img
	set ${prop}($name) 0
    } else {
	$b conf -image ay_Triangle_img
	set ${prop}($name) 1
    }
    eval $cmd
 return;
}
# updateOptionToggle

# addOptionToggle:
#  add UIE to show/hide additional UIEs
proc addOptionToggle { w prop name txt cmd } {
    global $prop ay ayprefs
    set bw 1
    set hl [expr round($ay(GuiScale))]
    set f [frame $w.f${name} -relief sunken -bd $bw]
    set b [button $w.f${name}.b -compound right -text $txt \
	       -image ay_TriangleR_img -bd 1 -highlightthickness $hl \
	       -command "updateOptionToggle $w $prop $name $cmd"]
    if { [info exists ${prop}($name)] } {
	eval "set v \$${prop}($name)"
	if { $v != 0 } {
	    $b conf -image ay_Triangle_img
	}
    }
    if { $ay(ws) == "X11" } {
	$b conf -height [expr round(8 * $ay(GuiScale))]
    }
    if { $ay(ws) == "Aqua" } {
	$b conf -height [expr round(11 * $ay(GuiScale))]
    }
    pack $b -in $f -side left -fill x -expand yes
    pack $f -in $w -side top -fill x
 return;
}
# addOptionToggle


# addScriptProperty:
#  manage the property of a Script object script, by:
#  * creating the appropriate data arrays in safe and main interpreter;
#  * arranging for all parameters from params spec to be saved between
#    multiple instances of the script object and to scene files;
#  * creating and populating the property GUI, based on params spec;
#  * copying the parameters from property GUI to designated variables
#    for easy perusal by the script object code;
#  * adding a NP tag to the script object so that the (new) property
#    GUI is visible to the user.
#  Even though the name of this procedure suggests otherwise, it can
#  and should be invoked on every run of the script objects code,
#  (unlike code that manages data arrays and GUI manually).
#
# Parameters:
#
# base  is the base name of the property, to be appearing in the list
#       of properties for this script object
# params  is a list of parameter specifications, where each parameter
#         specification is a list of three components:
#         {Name Defaultvalue ScriptContextVariableName}
#         * Name  is the string to be used in the property GUI as parameter
#           name,
#         * Defaultvalue  is the initial value to be used for this parameter,
#           the type of the default value determines the type of property GUI
#           element to be created:
#           - simple double and integer values lead to standard number
#             parameter GUI elements (addParam),
#           - true or false lead to check boxes, but the values in the script
#             variable will still be 1 or 0 (and not true or false),
#           - menu components will be created, when the default value is a list,
#             each element of this list is a string to display in the menu,
#             the zero based index in the list is the integer value to be put
#             into the corresponding script variable, the first entry from
#             this list is the default
#           - other types of values lead to simple string entry fields
#         * ScriptContextVariableName  is a short variable name where the value
#           from the property GUI element will be stored for use in the script
#
# Example:
#
# addScriptProperty CSphereAttr {{Radius 1.0 r} {ZMin -1.0 zi} {ZMax 1.0 za}}
#
# creates a property GUI for three floating point parameters;
# the script code can access the values via the global variables
# r, zi, and za respectively
#
# addScriptProperty CSphereAttr {{Radius {Small Medium Large} r}}
#
# creates a property GUI for one menu parameter, the variable r
# will be set to 0, 1, or 2, depending on menu selection
proc addScriptProperty { base params } {
    global ${base}Data

    if { ![aySafeInterp eval info exists ::${base}Data] } {
	foreach param $params {
	    if { [llength [lindex $param 1]] > 1 } {
		aySafeInterp eval set ::${base}Data([lindex $param 0]) 0
	    } else {
		if { [string is boolean [lindex $param 1]] } {
		    if { [lindex $param 1] } {
		    aySafeInterp eval set ::${base}Data([lindex $param 0]) 1
		    } else {
		    aySafeInterp eval set ::${base}Data([lindex $param 0]) 0
		    }
		} else {
		    aySafeInterp eval set ::${base}Data([lindex $param 0])\
			[lindex $param 1]
		}
	    }
	    aySafeInterp eval lappend ::${base}Data(SP) [lindex $param 0]
	}
    }
    foreach param $params {
       aySafeInterp eval\
	"set ::[lindex $param 2] \$${base}Data([lindex $param 0])"
    }
    if { ![info exists ::${base}GUI] } {
	array set ${base}Data [aySafeInterp eval array get ::${base}Data]
	set w [addPropertyGUI $base "" ""]
	addVSpace $w v1 2
	foreach param $params {
	    if { [llength [lindex $param 1]] > 1 } {
		addMenu $w ${base}Data [lindex $param 0] [lindex $param 1]
		continue;
	    }
	    if { [string is double [lindex $param 1]] } {
		addParam $w ${base}Data [lindex $param 0]
		continue;
	    }
	    if { [string is boolean [lindex $param 1]] } {
		addCheck $w ${base}Data [lindex $param 0] {1 0}
		continue;
	    }
	    addString $w ${base}Data [lindex $param 0]
	}
    }

    aySafeInterp eval sP

    if { ! [hasTag NP ${base}] } {
	addTag NP ${base}
    }
    if { ! [hasTag DA ${base}Data] } {
	setTag DA ${base}Data
    }

    aySafeInterp eval "selOb -clear"

 return;
}
# addScriptProperty
