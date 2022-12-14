# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2005 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# olb.tcl - the object listbox


#
proc olb_open { w } {
global ay ayprefs

set ay(lb) 1

frame $w.flb
set f $w.flb

# label
label $f.la -text "Objects:" -padx 0 -pady 0

# listbox
set hl [expr round($ay(GuiScale))]
set ay(olb) $f.li
listbox $f.li -width 14 -height 10 -selectmode extended\
	-highlightthickness $hl\
	-yscrollcommand {global ay; $ay(olbs) set}\
	-exportselection 0 -activestyle none

bind $f.li <Double-ButtonPress-1> {
    global ay
    set list [$ay(olb) curselection]
    eval [subst "goDown $list"]
    cS
    olb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
}

bind $f.li <<ListboxSelect>> { after 150 olb_select }

bind $f.li <Return> { olb_select }

bind $f.li <ButtonRelease-1> {
    global ay
    focus $ay(olb)
}


bind $f.li <ButtonPress-4> {
    global ay
    $ay(olb) yview scroll -1 pages
    break
}

bind $f.li <ButtonPress-5> {
    global ay
    $ay(olb) yview scroll 1 pages
    break
}

global tcl_platform AYWITHAQUA
if { ($tcl_platform(platform) == "windows") || $AYWITHAQUA } {
    bind $f.li <MouseWheel> {
	global ay
	if { %D < 0.0 } {
	    $ay(olb) yview scroll 1 pages
	} else {
	    $ay(olb) yview scroll -1 pages
	}
	break
    }
}

bind $f.li <Down> "selNPFL 0;break"
bind $f.li <Shift-Down> "selAdd 1;break"
bind $f.li <Shift-End> "selAdd 4;break"
bind $f.li <Up> "selNPFL 1;break"
bind $f.li <Shift-Up> "selAdd 0;break"
bind $f.li <Shift-Home> "selAdd 3;break"
bind $f.li <Home> "selNPFL 2;break"
bind $f.li <End> "selNPFL 3;break"

bind $f.li <Right> "$f.b;uS;break"


bind $f.li <Alt-Key-Up> {selMUD 1;break}

bind $f.li <Alt-Key-Down> {selMUD 0;break}


# initial focus management (plb_update/plb_focus change this again)
if { !$::ayprefs(SingleWindow) } {
    bind $f.li <Key-Tab> "focus .fl.con.console;break"
    bind $f.li <Shift-Tab> "focus .fv.fViews.fview2;break"
} else {
    bind $f.li <Key-Tab> "focus .fu.fMain.fview3;break"
}

#bind $f.li <KeyRelease-space> { olb_select }

# olb context menu
set m [menu $ay(olb).popup -tearoff 0]
$m add command -label "Switch to Tree" -command tree_toggle\
    -underline 0
$m add separator
$m add command -label "Shuffle Up <Alt-Up>"\
    -command "selMUD 1"
$m add command -label "Shuffle Down <Alt-Down>"\
    -command "selMUD 0"
$m add separator
$m add command -label "Deselect Object" -command {
    global ay
    cS
    plb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -underline 1
$m add separator
$m add command -label "Copy Object" -command "\$ay(editmenu) invoke 0"\
    -underline 0
$m add command -label "Cut Object" -command "\$ay(editmenu) invoke 1"\
    -underline 2
$m add command -label "Paste Object" -command "\$ay(editmenu) invoke 2"\
    -underline 0
#$m add command -label "Paste (Move)" -command "cmovOb;uS;rV"
$m add separator
$m add command -label "Delete Object" -command "\$ay(editmenu) invoke 3"\
    -underline 0

$m add separator
$m add command -label "Help on Object" -command "\$ay(helpmenu) invoke 1"\
    -underline 0

if { $ay(ws) == "Aqua" && $ayprefs(SwapMB) } {
    bind $f.li <ButtonPress-2> "tk_popup $m %X %Y"
} else {
    bind $f.li <ButtonPress-3> "tk_popup $m %X %Y"
}

# scrollbar
set ay(olbs) $f.s
scrollbar $f.s -command {global ay; $ay(olb) yview} -takefocus 0
pack $f.la -in $f -side top -fill x
pack $f.li -in $f -side left -fill both -expand yes
pack $f.s -in $f -side left -fill y -expand no

# focus management
bind $f.la <ButtonPress-1> "focus -force $ay(olb)"

# switch from listbox to tree
bind $f.la <Double-1> tree_toggle

# see ms.tcl ms_initmainlabels
set ay(olbl) $f.la

# buttons
frame $w.fb -highlightthickness 1

set f $w.fb
frame $f.f1
set f $f.f1
button $f.bnon -text "None" -padx 0 -pady 0 -command {
    global ay
    cS
    plb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -takefocus 0 -highlightthickness 0

button $f.bup -text "Up" -padx 0 -pady 0 -command {
    global ay;
    cS
    goUp
    olb_update
    if { [info exists ay(lastdown)] && ([llength $ay(lastdown)] > 0) &&
	 ([lindex $ay(lastdown) end] != "") } {
	set lb $ay(olb)
	$lb selection set [lindex $ay(lastdown) end]
	set ay(lastdown) [lreplace $ay(lastdown) end end]
	olb_select
    } else {
	if { $ay(need_redraw) == 1 } {
	    rV
	}
    }
} -takefocus 0 -highlightthickness 0

bind $ay(olb) <Left> "$f.bup invoke; break"
set ay(olbbup) $f.bup

pack $f.bnon $f.bup -in $f -side top -fill x -expand yes

set f $w.fb
frame $f.f2
set f $f.f2

button $f.ball -text "All" -padx 0 -pady 0 -command {
    global ay
    set lb $ay(olb)
    if { [$lb index end] > 1 } {
	$lb selection clear 0 end
	$lb selection set 1 end
	set selection [$lb curselection]
	eval [subst "selOb -lb $selection"]
	olb_update
    }
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -takefocus 0 -highlightthickness 0
set ay(olbball) $f.ball

button $f.btop -text "Top" -padx 0 -pady 0 -command {
    cS
    goTop
    olb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -takefocus 0 -highlightthickness 0

pack $f.ball $f.btop -in $f -side top -fill x -expand yes

set f $w.fb
frame $f.f3
set f $f.f3

button $f.binv -text "Inv" -padx 0 -pady 0 -command {
    global ay
    set selected [$ay(olb) curselection]
    $ay(olb) selection set 0 end
    foreach element $selected {
	$ay(olb) selection clear $element
    }
    olb_select
} -takefocus 0 -highlightthickness 0

button $f.bdwn -text "Down" -padx 0 -pady 0 -command {
    global ay
    set list [$ay(olb) curselection]
    lappend ay(lastdown) [lindex $list 0]
    eval [subst "goDown $list"]
    cS
    olb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -takefocus 0 -highlightthickness 0

bind $ay(olb) <Right> "$f.bdwn invoke; break"
set ay(olbbdwn) $f.bdwn

pack $f.binv $f.bdwn -in $f -side top -fill x -expand yes

set f $w.fb
pack $f.f1 -in $f -side left -fill x -expand yes
pack $f.f2 -in $f -side left -fill x -expand yes
pack $f.f3 -in $f -side left -fill x -expand yes
pack $w.fb -in $w -side bottom -fill x
update
pack $w.flb -in $w -side left -fill both -expand yes

}
# olb_open


#
proc olb_close { w } {
    global ay

    set ay(lb) 0

    destroy $w.flb
    destroy $w.fb
 return;
}
# olb_close


#
proc olb_update { } {
    global ay

    set lb $ay(olb)

    set selobj [$lb curselection]

    update

    # delete all current entries
    $lb delete 0 end

    # get current objects
    getLevel curlevel

    eval [subst "$lb insert end $curlevel"]

    if { $selobj != "" } {
	foreach i $selobj {
	    $lb selection set $i
	}
	olb_select
    } else {
	plb_update
    }
}
# olb_update


# select from object listbox selection
proc olb_select { } {
    global ay

    if { $ay(sellock) == 1 } {
	bell; return;
    } else {
	set ay(sellock) 1
    }

    set lb $ay(olb)
    set selection [$lb curselection]

    eval "selOb -lb $selection"

    plb_update

    if { $ay(need_redraw) == 1 } {
	rV
    }

    set ay(sellock) 0

 return;
}
# olb_select


# olb_sync:
# synchronize listbox widget selection state with current selOb selection
proc olb_sync { lb } {
    global ay
    getSel sel
    if { [llength $sel] > 0 } {
	$ay(olb) selection clear 0 end
	foreach s $sel {
	    if { $ay(CurrentLevel) != "root" } {
		incr s
	    }
	    $lb selection set $s
	}
    } else {
	cS
    }
 return;
}
# olb_sync
