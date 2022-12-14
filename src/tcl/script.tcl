# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2004 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# script.tcl - script objects Tcl code

proc init_Script { } {
    global ay ayprefs Script_props ScriptAttr ScriptAttrData

    set Script_props { Attributes Tags ScriptAttr }

    array set ScriptAttr {
	arr   ScriptAttrData
	sproc script_set
	gproc script_get
	w     fScriptAttr
    }
    # array ScriptAttr

    array set ScriptAttrData {
	Active 0
	Type 0
	Script ""
	BoundaryNames { "U0" "U1" "V0" "V1" }
	BevelsChanged 0
	CapsChanged 0
    }
    # array ScriptAttrData

    # create ScriptAttr-UI:
    set w [frame $ay(pca).$ScriptAttr(w)]
    addVSpace $w s1 2
    addCheck $w ScriptAttrData Active
    addMenu $w ScriptAttrData Type [list Run Create Modify]
    pack [text $w.tScript -undo 1 -width 45 -height 15] -expand yes -fill x
    set t $w.tScript
    eval [subst "bindtags $t \{$t Text all\}"]
    bind $t <Key-Escape> "resetFocus;break"

    # create resize handle for text widget
    if { $ay(ws) == "Windows" } {
	resizeHandle:Create $w.rsh $t -bg SystemWindow
    } else {
	resizeHandle:Create $w.rsh $t
    }

    bind $w <Configure> "+after idle \"resizeHandle:PlaceHandle $w.rsh $t;\""

    if { $ay(ws) == "Aqua" } {
	$t conf -relief sunken -bd 2
    }

    # create popup menu
    set m [menu $t.popup -tearoff 0]
    $m add command -label "Clear All" -command "$t delete 1.0 end"
    $m add command -label "Paste (Replace)" -command "_pasteToText $t"

    set types {{"Script Files" {".tcl" ".js" ".lua"}}}
    $m add command -label "Load from file" -command \
	"_loadToText $t {$types}"

    # bind popup menu
    set mb 3
    if { $ay(ws) == "Aqua" && $ayprefs(SwapMB) } {
	set mb 2
    }
    bind $t <$mb> {
	global ay ScriptAttr
	set ay(xy) [winfo pointerxy .]
	tk_popup $ay(pca).${ScriptAttr(w)}.tScript.popup\
	    [lindex $ay(xy) 0] [lindex $ay(xy) 1]
	break
    }
    # bind

    bind $t <Shift-F10> [bind $t <$mb>]
    bind $t <Alt-Right> [bind $ay(pca) <Alt-Right>]
    bind $t <Alt-Left> [bind $ay(pca) <Alt-Left>]

 return;
}
# init_Script


# Tcl -> C
proc script_set { } {
    global ay ScriptAttr ScriptAttrData

    set t $ay(pca).$ScriptAttr(w).tScript

    set tags [$t tag names]
    if { [lsearch -exact $tags errtag] != -1 } {
	$t tag delete errtag
    }

    set ScriptAttrData(Script) [$t get 1.0 end]
    setProp

    if { [$ay(plb) get [$ay(plb) curselection]] == "ScriptAttr" } {
	plb_update
	after idle "event generate $ay(plb) <<ListboxSelect>>"
    }
 return;
}
# script_set

# C -> Tcl
proc script_get { } {
    global ay ScriptAttr ScriptAttrData

    set t $ay(pca).$ScriptAttr(w).tScript
    getProp
    set errrange ""
    set tags [$t tag names]
    if { [lsearch -exact $tags errtag] != -1 } {
	set errrange [$t tag range errtag]
    }
    $t delete 1.0 end
    $t insert 1.0 $ScriptAttrData(Script)
    if { $ScriptAttrData(Script) != "" } {
	$t delete "end - 1 chars"
    }

    if { $errrange != "" } {
	eval [subst "$t tag add errtag $errrange"]
    }

    after 100 "resizeHandle:PlaceHandle $ay(pca).$ScriptAttr(w).rsh \
      $ay(pca).$ScriptAttr(w).tScript"

 return;
}
# script_get

# script_disable:
#  ask user for permission to disable all script objects loaded
#  with a scene file
proc script_disable { } {
    global ay ayprefs

    if { $ayprefs(AskScriptDisable) && !$ay(askedscriptdisable) } {
	set t "Disable Scripts?"
	set m "Scene contains Script objects or tags, disable them?"

	if { $ayprefs(FixDialogTitles) == 1 } {
	    set m "$t\n\n$m"
	}

	set answer [tk_messageBox -title $t -type yesno -icon warning -message $m]
	if { $answer == "yes" } {
	    set ay(scriptdisable) 1
	} else {
	    set ay(scriptdisable) 0
	}
	set ay(askedscriptdisable) 1
    }

 return;
}
# script_disable

# enable all script tags and objects
proc script_enable { } {
    global ay ayprefs

    goTop
    selOb
    cS
    plb_update

    # activate all script object scripts
    forAll -type script {
	script_get
	set ::ScriptAttrData(Active) 1
	script_set
    }

    # activate script tags by renaming all DBNS/DANS tags to BNS/ANS tags
    forAll {
	set tagnames ""; set tagvals ""
	getTags tagnames tagvals
	if { $tagnames != "" } {
	    set tagindex 0
	    foreach tagname $tagnames {
		if { $tagname == "DBNS" } {
		    setTags -index $tagindex "BNS" [lindex $tagvals $tagindex]
		}
		if { $tagname == "DANS" } {
		    setTags -index $tagindex "ANS" [lindex $tagvals $tagindex]
		}
		incr tagindex
	    }
	}
    }

    goTop
    selOb
    notifyOb

 return;
}
# script_enable


proc resizeHandle:ButtonPress-1 {win resizeWin X Y} {
    upvar #0 _resizeHandle$win ar
    set ar(startX) $X
    set ar(startY) $Y
    set ar(oldWidth) [$resizeWin cget -width]
    set ar(oldHeight) [$resizeWin cget -height]
}

proc resizeHandle:B1-Motion {win resizeWin X Y} {
    upvar #0 _resizeHandle$win ar
    set xDiff [expr {$X - $ar(startX)}]
    set yDiff [expr {$Y - $ar(startY)}]

    set newWidth [expr {$ar(oldWidth) + $xDiff / $ar(cw)}]
    set newHeight [expr {$ar(oldHeight) + $yDiff / $ar(ch)}]

    if {$newWidth < 2 || $newHeight < 2} {
	return
    }
    if { $newWidth != $ar(oldWidth) || $newHeight != $ar(oldHeight) } {
	$resizeWin conf -width $newWidth -height $newHeight
	after idle "resizeHandle:PlaceHandle $win $resizeWin"
    }
}

proc resizeHandle:PlaceHandle {win resizeWin} {
    set x [expr [winfo x $resizeWin]+[winfo width $resizeWin]-3]
    set y [expr [winfo y $resizeWin]+[winfo height $resizeWin]-3]

    place $win -in [winfo parent $win] -anchor se -x $x -y $y
}

proc resizeHandle:Destroy {win} {
    upvar #0 _resizeHandle$win ar
    # catch, because ar may not be set
    catch {array unset ar}
}

proc resizeHandle:Create {win resizeWin args} {
    upvar #0 _resizeHandle$win ar
    eval label [concat $win $args -image ay_Resizehandle_img]
    resizeHandle:PlaceHandle $win $resizeWin
    bind $win <ButtonPress-1> \
	"resizeHandle:ButtonPress-1 $win $resizeWin %X %Y"
    bind $win <B1-Motion> \
	"resizeHandle:B1-Motion $win $resizeWin %X %Y"
    bind $win <ButtonRelease-1> \
	"resizeHandle:PlaceHandle $win $resizeWin"
    bind $win <Destroy> "resizeHandle:Destroy $win"
    #bind $resizeWin <Configure> "+resizeHandle:PlaceHandle $win $resizeWin;"

    set font [$win cget -font]
    set ar(cw) [expr [font measure $font "A"]]
    set ar(ch) [font metrics $font -linespace]

 return $win
}
