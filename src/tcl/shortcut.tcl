# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2009 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# shortcut.tcl - procedures to establish key bindings


# repctrl:
#  helper to replace "Ctrl" by "Control"
proc repctrl { string } {
    regsub -all "Ctrl" $string "Control" string
    return $string
}
# repctrl

# remkpkr:
#  helper to remove "KeyPress-"/"KeyRelease-"
proc remkpkr { string } {
    regsub -all "KeyPress-" $string "" string
    regsub -all "KeyRelease-" $string "" string
    return $string
}
# remkpkr

# _makevis:
#  helper to make shortcut
proc _makevis { string } {
    set string [remkpkr $string]
    set string [string map [shortcut_getmappings] $string]
    return $string
}
# _makevis

# shortcut_swapmb:
#  swap mouse button numbers from Win32/X11 style to Aqua style
#  in shortcuts designated by ayprefs(SwapMBSC)
proc shortcut_swapmb { } {
 global ay ayprefs

    if { ($ayprefs(SwapMB) == 1) && ($ayprefs(WindowSystem) != $ay(ws)) } {
	foreach shortcut $ayprefs(SwapMBSC) {
	    eval [subst "set val \$::$shortcut"]
	    set val [string map { 2 3 3 2 } $val]
	    eval [subst "set ::$shortcut $val"]
	}
    }

 return;
}
# shortcut_swapmb


# shortcut_fkeys:
#  Setup Function-Key-Bindings for window w
proc shortcut_fkeys { w } {
 global aymainshortcuts tcl_platform

    # fix <Alt-F4> firing the binding for <F4>
    if { $tcl_platform(platform) == "windows" } {
	bindtags $w [linsert [bindtags $w] 0 altf4]
	bind altf4 <Alt-F4> [bind all <Alt-F4>]
	bind $w <Alt-F4> [bind all <Alt-F4>]
    }

    bind $w <[repctrl $aymainshortcuts(SwLazyNotify)]>\
     { prefs_toggleLazyNotification }
    bind $w <[repctrl $aymainshortcuts(SwNURBSWire)]>\
     { prefs_toggleSurfaceWire }
    bind $w <[repctrl $aymainshortcuts(SetSTP)]>\
     { prefs_setSamplingTolerance 1 }
    bind $w <[repctrl $aymainshortcuts(SetSTL)]>\
     { prefs_setSamplingTolerance 0 }
    bind $w <[repctrl $aymainshortcuts(Update)]>\
     {
	 ayError 4 $aymainshortcuts(Update) "Update!"
	 notifyOb -all; uS 1 1; rV "" 1;
     }

 return;
}
# shortcut_fkeys


# shortcut_main:
#  Setup Keybindings for the Main Window
proc shortcut_main { w } {
    global ay aymainshortcuts ayviewshortcuts AYWITHAQUA tcl_platform

    set m $ay(filemenu)
    bind $w <[repctrl $aymainshortcuts(New)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $aymainshortcuts(New)]
    bind $w <[repctrl $aymainshortcuts(Replace)]> "$m invoke 2;break"
    $m entryconfigure 2 -accelerator [remkpkr $aymainshortcuts(Replace)]
    bind $w <[repctrl $aymainshortcuts(Insert)]> "$m invoke 3;break"
    $m entryconfigure 3 -accelerator [remkpkr $aymainshortcuts(Insert)]
    bind $w <[repctrl $aymainshortcuts(SaveAs)]> "$m invoke 5;break"
    $m entryconfigure 5 -accelerator [remkpkr $aymainshortcuts(SaveAs)]
    bind $w <[repctrl $aymainshortcuts(Save)]> "$m invoke 6;break"
    $m entryconfigure 6 -accelerator [remkpkr $aymainshortcuts(Save)]
    bind $w <[repctrl $aymainshortcuts(ExportRIB)]> "$m.ex invoke 0;break"
    $m.ex entryconfigure 0 -accelerator [remkpkr $aymainshortcuts(ExportRIB)]

    bind $w <[repctrl $aymainshortcuts(MRU1)]> "$m invoke 16;break"
    $m entryconfigure 16\
	    -accelerator [remkpkr $aymainshortcuts(MRU1)]
    bind $w <[repctrl $aymainshortcuts(MRU2)]> "$m invoke 17;break"
    $m entryconfigure 17\
	    -accelerator [remkpkr $aymainshortcuts(MRU2)]
    bind $w <[repctrl $aymainshortcuts(MRU3)]> "$m invoke 18;break"
    $m entryconfigure 18\
	    -accelerator [remkpkr $aymainshortcuts(MRU3)]
    bind $w <[repctrl $aymainshortcuts(MRU4)]> "$m invoke 19;break"
    $m entryconfigure 19\
	    -accelerator [remkpkr $aymainshortcuts(MRU4)]

    bind $w <[repctrl $aymainshortcuts(Quit)]> "$m invoke 21;break"
    $m entryconfigure 21 -accelerator [remkpkr $aymainshortcuts(Quit)]

    bind $w <[repctrl $aymainshortcuts(Apply)]> "$ay(appb) invoke;break"
    bind $w <[repctrl $aymainshortcuts(Reset)]> "$ay(rstb) invoke;break"

    set m $ay(editmenu)
    bind $w <[repctrl $aymainshortcuts(Copy)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $aymainshortcuts(Copy)]
    bind $w <[repctrl $aymainshortcuts(Cut)]> "$m invoke 1;break"
    $m entryconfigure 1 -accelerator [remkpkr $aymainshortcuts(Cut)]
    bind $w <[repctrl $aymainshortcuts(Paste)]> "$m invoke 2;break"
    $m entryconfigure 2 -accelerator [remkpkr $aymainshortcuts(Paste)]

    bind $w <[repctrl $aymainshortcuts(SelAll)]> "$m invoke 5;break"
    $m entryconfigure 5 -accelerator [remkpkr $aymainshortcuts(SelAll)]
    bind $w <[repctrl $aymainshortcuts(SelNone)]> "$m invoke 6;break"
    $m entryconfigure 6 -accelerator [remkpkr $aymainshortcuts(SelNone)]

    bind $w <[repctrl $aymainshortcuts(CopyP)]> "$m invoke 8;break"
    $m entryconfigure 8 -accelerator [remkpkr $aymainshortcuts(CopyP)]
    bind $w <[repctrl $aymainshortcuts(ICopyP)]> "$m invoke 9;break"
    $m entryconfigure 9 -accelerator [remkpkr $aymainshortcuts(ICopyP)]
    bind $w <[repctrl $aymainshortcuts(PasteP)]> "$m invoke 10;break"
    $m entryconfigure 10 -accelerator [remkpkr $aymainshortcuts(PasteP)]

    bind $w <[repctrl $aymainshortcuts(Undo)]> "$m invoke 12;break"
    $m entryconfigure 12 -accelerator [remkpkr $aymainshortcuts(Undo)]
    bind $w <[repctrl $aymainshortcuts(Redo)]> "$m invoke 13;break"
    $m entryconfigure 13 -accelerator [remkpkr $aymainshortcuts(Redo)]

    bind $w <[repctrl $aymainshortcuts(Material)]> "$m invoke 15;break"
    $m entryconfigure 15 -accelerator [remkpkr $aymainshortcuts(Material)]

    bind $w <[repctrl $aymainshortcuts(Master)]> "$m invoke 16;break"
    $m entryconfigure 16 -accelerator [remkpkr $aymainshortcuts(Master)]

    bind $w <[repctrl $aymainshortcuts(Search)]> "$m invoke 18;break"
    $m entryconfigure 18 -accelerator [remkpkr $aymainshortcuts(Search)]

    bind $w <[repctrl $aymainshortcuts(Prefs)]> "$m invoke 20;break"
    $m entryconfigure 20 -accelerator [remkpkr $aymainshortcuts(Prefs)]

    set m $ay(toolsmenu)
    bind $w <[repctrl $aymainshortcuts(LastTool)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $aymainshortcuts(LastTool)]

    bind $w <[repctrl $aymainshortcuts(LastToolP)]> "\
     if \{ \$ay(lasttool) != \"\" \} \{ eval \$ay(lasttool) \}"

    bind $w <[repctrl $aymainshortcuts(Hide)]>\
	"$m invoke 12;break"
    $m entryconfigure 12 -accelerator [remkpkr $aymainshortcuts(Hide)]

    bind $w <[repctrl $aymainshortcuts(Show)]>\
	"$m invoke 14;break"
    $m entryconfigure 14 -accelerator [remkpkr $aymainshortcuts(Show)]

    bind $w <[repctrl $aymainshortcuts(Convert)]>\
	"$m invoke 16;break"
    $m entryconfigure 16 -accelerator [remkpkr $aymainshortcuts(Convert)]

    bind $w <[repctrl $aymainshortcuts(ConvertP)]>\
	"$m invoke 17;break"
    $m entryconfigure 17 -accelerator [remkpkr $aymainshortcuts(ConvertP)]

    set m $ay(helpmenu)
    bind $w <[repctrl $aymainshortcuts(Help)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $aymainshortcuts(Help)]

    bind $w <[repctrl $aymainshortcuts(Shortcuts)]> "$m invoke 3;break"
    $m entryconfigure 3 -accelerator\
	[_makevis [remkpkr $aymainshortcuts(Shortcuts)]]

    bind $w <$aymainshortcuts(ContextMod)-Key-$aymainshortcuts(Help)>\
	"$m invoke 1"

    set m $ay(specialmenu)
    bind $w <[repctrl $aymainshortcuts(Zap)]> zap
    $m entryconfigure 21 -accelerator [remkpkr $aymainshortcuts(Zap)]

    bind $w <Map> unzap

    bind $w <[repctrl $aymainshortcuts(RebuildTree)]> {
	global ay
	if { $ay(lb) == 0 } { tree_reset }
    }

    # bind function keys
    shortcut_fkeys $w

    if { $w  == "." } {
	bind $w <Enter> {
	    global ayprefs
	    if { $ayprefs(AutoFocus) == 1 } {
		if { "%W" == "[winfo toplevel %W]" } {
		    focus %W
		}
	    }
	}
    }

    bind $w <Left> {
	global ay
	if { $ay(lb) == 0 } {
	    set tree $ay(tree)
	    set cl $ay(CurrentLevel)
	    if { $cl == "root" } {
		break
	    }
	    set i [string last ":" $cl]
	    set newcl [string range $cl 0 [expr ${i}-1]]
	    goUp
	    set ay(SelectedLevel) $newcl
	    $tree selection set $cl
	    $tree see $cl
	    treeSelect $cl
	    tree_paintLevel $newcl
	    set ay(CurrentLevel) $newcl
	    plb_update
	    rV
	} else {
	    $ay(olbbup) invoke
	    break
	}
	# if
    }
    # bind

    bind $w <Right> {
	global ay
	if { $ay(lb) == 0 } {
	    set tree $ay(tree)
	    set sel [$tree selection get]
	    if { $sel == "" } {
		break
	    }
	    set osel $sel
	    set sel [lindex $sel 0]
	    $tree selection set $sel
	    treeSelect $sel
	    if { ! [hasChild] && [isParent] } {
		$tree itemconfigure $sel -open 1
		set i [string range $sel [expr [string last ":" $sel]+1] end]
		if { [string last ":" $sel] != 4 } {incr i}
		goDown $i
		set ay(SelectedLevel) $sel
		$tree selection clear
		tree_paintLevel $sel
		set ay(CurrentLevel) $sel
		plb_update
		rV
		break
	    } else {
		if { ! [isParent] } {
		    eval "$tree selection set $osel"
		    eval "treeSelect $osel"
		    break;
		}
	    }
	    $tree itemconfigure $sel -open 1
	    goDown $sel
	    set ay(SelectedLevel) $sel
	    $tree selection set ${sel}:0
	    $tree see ${sel}:0
	    treeSelect ${sel}:0
	    tree_paintLevel $sel
	    set ay(CurrentLevel) $sel
	    plb_update
	    rV
	} else {
	    $ay(olbbdwn) invoke
	    break
	}
	# if
    }
    # bind

    bind $w <Down> "selNPFL 0"
    bind $w <Shift-Down> "selAdd 1"
    bind $w <Shift-End> "selAdd 4"
    bind $w <Up> "selNPFL 1"
    bind $w <Shift-Up> "selAdd 0"
    bind $w <Shift-Home> "selAdd 3"
    bind $w <Home> "selNPFL 2"
    bind $w <End> "selNPFL 3"

    bind $w <Alt-Key-Up> {selMUD 1;break}
    bind $w <Alt-Key-Down> {selMUD 0;break}

    bind $w <space> {
	global ay
	if { $ay(lb) == 0 } {
	    set tree $ay(tree)
	    set sel ""
	    set sel [$tree selection get]
	    foreach node $sel {
		if { ! [$tree itemcget $node -open] } {
		    $tree itemconfigure $node -open 1
		} else {
		    $tree itemconfigure $node -open 0
		}
	    }
	    # foreach
	}
	# if
    }
    # bind

    # open/close/toggle whole subtrees
    if { $ay(lb) == 0 } {
	set t $ay(tree)
	bind $t <Shift-space> "tree_toggleTree 0;break"
	bind $t <plus> "tree_toggleTree 1;break"
	bind $t <minus> "tree_toggleTree 2;break"
	bind $t <Key-$aymainshortcuts(ExpandAll)> "tree_expand \$ay(tree)"
	bind $t <Key-$aymainshortcuts(CollapseAll)> "tree_collapse \$ay(tree)"
    }

    bind $w <[repctrl $aymainshortcuts(SProp0)]> "plb_showprop 0"
    bind $w <[repctrl $aymainshortcuts(SProp1)]> "plb_showprop 1"
    bind $w <[repctrl $aymainshortcuts(SProp2)]> "plb_showprop 2"
    bind $w <[repctrl $aymainshortcuts(SProp3)]> "plb_showprop 3"
    bind $w <[repctrl $aymainshortcuts(SProp4)]> "plb_showprop 4"
    bind $w <[repctrl $aymainshortcuts(SProp5)]> "plb_showprop 5"
    bind $w <[repctrl $aymainshortcuts(SProp6)]> "plb_showprop 6"
    bind $w <[repctrl $aymainshortcuts(SProp7)]> "plb_showprop 7"
    bind $w <[repctrl $aymainshortcuts(SProp8)]> "plb_showprop 8"
    bind $w <[repctrl $aymainshortcuts(SProp9)]> "plb_showprop 9"

    bind $w <[repctrl $aymainshortcuts(SProp00)]> "plb_showprop 0"
    bind $w <[repctrl $aymainshortcuts(SProp11)]> "plb_showprop 1"
    bind $w <[repctrl $aymainshortcuts(SProp22)]> "plb_showprop 2"
    bind $w <[repctrl $aymainshortcuts(SProp33)]> "plb_showprop 3"
    bind $w <[repctrl $aymainshortcuts(SProp44)]> "plb_showprop 4"
    bind $w <[repctrl $aymainshortcuts(SProp55)]> "plb_showprop 5"
    bind $w <[repctrl $aymainshortcuts(SProp66)]> "plb_showprop 6"
    bind $w <[repctrl $aymainshortcuts(SProp77)]> "plb_showprop 7"
    bind $w <[repctrl $aymainshortcuts(SProp88)]> "plb_showprop 8"
    bind $w <[repctrl $aymainshortcuts(SProp99)]> "plb_showprop 9"

    bind $w <[repctrl $aymainshortcuts(ContextMenu)]> \
	"if { \$ay(lb) == 1 } {winOpenPopup \$ay(olb)} else {winOpenPopup \$ay(tree)};break"
    bind $w <[repctrl $aymainshortcuts(ContextMenu2)]> \
	"if { \$ay(lb) == 1 } {winOpenPopup \$ay(olb)} else {winOpenPopup \$ay(tree)};break"

    # on Aqua, <Command-q> quits the application
    if { $AYWITHAQUA == 1 } {
	bind all <Command-q> exit
    }

    if { $tcl_platform(platform) == "windows" } {
	set m $ay(filemenu)
	bind all <Alt-F4> "$m invoke 21;break"
	bind . <Alt-F4> "$m invoke 21;break"
    }

    if { [winfo exists $ay(treecm).tree] } {
	$ay(treecm).tree entryconfigure 0\
	    -accelerator [remkpkr $aymainshortcuts(RebuildTree)]
    }

    # autofocus bindings
    if { $::ayprefs(SingleWindow) } {
	if { $::ay(lb) == 1 } {
	    bind $::ay(olb) <Enter> {
		if { $ayprefs(AutoFocus) == 1 } {
		    focus $::ay(olb)
		}
	    }
	} else {
	    bind $::ay(tree) <Enter> {
		if { $ayprefs(AutoFocus) == 1 } {
		    focus $::ay(tree)
		}
	    }
	}
	bind $::ay(plb) <Enter> {
	    if { $ayprefs(AutoFocus) == 1 } {
		focus $::ay(pca)
	    }
	}
	bind $::ay(pca) <Enter> {
	    if { $ayprefs(AutoFocus) == 1 } {
		focus $::ay(pca)
	    }
	}
	bind .fl.con.console <Enter> {
	    if { $ayprefs(AutoFocus) == 1 } {
		focus .fl.con.console
	    }
	}
    }
    # if

    bind $w <[repctrl $aymainshortcuts(View1)]> {
	if { $ayprefs(SingleWindow) } {
	    focus -force [lindex $ay(views) 0]
	}
    }

    bind $w <[repctrl $ayviewshortcuts(ZoomTO2)]> "viewsZoomTo"
    bind $w <[repctrl $ayviewshortcuts(ZoomAll)]> "viewsZoomTo all"

 return;
}
# shortcut_main


# shortcut_view:
# Setup menu- and main-keybindings for a 3D-View.
proc shortcut_view { w } {
    global ay ayprefs ayviewshortcuts aymainshortcuts AYWITHAQUA

    if { (! $AYWITHAQUA) || ([winfo toplevel $w] != $w) } {
	set viewm $w.fMenu.v.m
	set typem $w.fMenu.t.m
	set confm $w.fMenu.c.m
    } else {
	set viewm $w.menubar.mview
	set typem $w.menubar.mtype
	set confm $w.menubar.mconf
    }

    if { [winfo toplevel $w] != $w } {
	# internal view
	shortcut_main $w
    } else {
	# external view
	# add only some main window shortcuts
	set m $ay(helpmenu)
	bind $w <[repctrl $aymainshortcuts(Help)]> "$m invoke 0;break"
	bind $w <[repctrl $aymainshortcuts(Shortcuts)]> "$m invoke 3;break"

	set m $ay(editmenu)
	bind $w <[repctrl $aymainshortcuts(Undo)]> "$m invoke 12;break"
	bind $w <[repctrl $aymainshortcuts(Redo)]> "$m invoke 13;break"
	bind $w <[repctrl $aymainshortcuts(Material)]> "$m invoke 15;break"
	bind $w <[repctrl $aymainshortcuts(Master)]> "$m invoke 16;break"

	# object selection
	bind $w <Shift-Up> "selAdd 0; after idle \"focus %W\""
	bind $w <Shift-Home> "selAdd 3; after idle \"focus %W\""
	bind $w <Home> "selNPFL 2; after idle \"focus %W\""
	bind $w <End> "selNPFL 3; after idle \"focus %W\""
    }

    # view window shortcuts
    set m $viewm
    bind $w <[repctrl $ayviewshortcuts(QRender)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $ayviewshortcuts(QRender)]
    bind $w <[repctrl $ayviewshortcuts(Render)]> "$m invoke 1;break"
    $m entryconfigure 1 -accelerator [remkpkr $ayviewshortcuts(Render)]

    bind $w <[repctrl $aymainshortcuts(ExportRIB)]> "$m invoke 3;break"
    $m entryconfigure 3 -accelerator [remkpkr $aymainshortcuts(ExportRIB)]
    if { [string first ".view" $w] == 0 } {
	global AYENABLEPPREV
	if { $AYENABLEPPREV == 1 } { set tmp 14 } else { set tmp 11 }
	bind $w <[repctrl $ayviewshortcuts(Close)]> "$m invoke $tmp"
	$m entryconfigure $tmp -accelerator [remkpkr $ayviewshortcuts(Close)]
    }

    bind $w <[repctrl $ayviewshortcuts(Redraw)]> "$m invoke 5;break"
    $m entryconfigure 5 -accelerator [remkpkr $ayviewshortcuts(Redraw)]

    if { ([winfo toplevel $w] == $w) || $ayprefs(BindInternalViews) } {
	set m $typem
	bind $w <[repctrl $ayviewshortcuts(Front)]> "$m invoke 0;break"
	$m entryconfigure 0 -accelerator [remkpkr $ayviewshortcuts(Front)]
	bind $w <[repctrl $ayviewshortcuts(Side)]> "$m invoke 1;break"
	$m entryconfigure 1 -accelerator [remkpkr $ayviewshortcuts(Side)]
	bind $w <[repctrl $ayviewshortcuts(Top)]> "$m invoke 2;break"
	$m entryconfigure 2 -accelerator [remkpkr $ayviewshortcuts(Top)]
	bind $w <[repctrl $ayviewshortcuts(Persp)]> "$m invoke 4;break"
	$m entryconfigure 4 -accelerator [remkpkr $ayviewshortcuts(Persp)]
	bind $w <[repctrl $ayviewshortcuts(Trim)]> "$m invoke 6;break"
	$m entryconfigure 6 -accelerator [remkpkr $ayviewshortcuts(Trim)]
    }

    set m $confm
    bind $w <[repctrl $ayviewshortcuts(Auto)]> "$m invoke 0;break"
    $m entryconfigure 0 -accelerator [remkpkr $ayviewshortcuts(Auto)]

    #bind $w <[repctrl $ayviewshortcuts(Shade)]> "$m invoke 1"
    #$m entryconfigure 1 -accelerator $ayviewshortcuts(Shade)
    if { ([winfo toplevel $w] == $w) || $ayprefs(BindInternalViews) } {
	bind $w <[repctrl $ayviewshortcuts(Shade)]> "viewToggleDMode $w;break"
    }

    bind $w <[repctrl $ayviewshortcuts(DGrid)]> "$m invoke 11;break"
    $m entryconfigure 11 -accelerator [remkpkr $ayviewshortcuts(DGrid)]
    bind $w <[repctrl $ayviewshortcuts(UGrid)]> "$m invoke 12;break"
    $m entryconfigure 12 -accelerator [remkpkr $ayviewshortcuts(UGrid)]
    bind $w <[repctrl $ayviewshortcuts(SGrid)]> "$m invoke 13;break"
    $m entryconfigure 13 -accelerator [remkpkr $ayviewshortcuts(SGrid)]

    if { ([winfo toplevel $w] == $w) } {
	bind $w <[repctrl $ayviewshortcuts(Halve)]> "$m invoke 15;break"
	$m entryconfigure 15 -accelerator [remkpkr $ayviewshortcuts(Halve)]
	bind $w <[repctrl $ayviewshortcuts(Double)]> "$m invoke 16;break"
	$m entryconfigure 16 -accelerator [remkpkr $ayviewshortcuts(Double)]
    }
    if { ([winfo toplevel $w] == $w) || $ayprefs(BindInternalViews) } {
	$m entryconfigure 22 -accelerator [remkpkr $ayviewshortcuts(ZoomTO)]
	bind $w <[repctrl $ayviewshortcuts(ZoomTO)]> "$m invoke 22;break"
	$m entryconfigure 23 -accelerator [remkpkr $ayviewshortcuts(ZoomAll)]
	bind $w <[repctrl $ayviewshortcuts(ZoomAll)]> "$m invoke 23;break"
	$m entryconfigure 24 -accelerator [remkpkr $ayviewshortcuts(Align)]
	bind $w <[repctrl $ayviewshortcuts(Align)]> "$m invoke 24;break"
    } else {
	$m entryconfigure 22 -accelerator [remkpkr $ayviewshortcuts(ZoomTO2)]
	$m entryconfigure 23 -accelerator [remkpkr $ayviewshortcuts(ZoomAll)]
	$m entryconfigure 24 -accelerator [remkpkr $ayviewshortcuts(Align2)]
    }

    bind $w <[repctrl $ayviewshortcuts(ZoomTO2)]> "$m invoke 22;break"
    bind $w <[repctrl $ayviewshortcuts(ZoomAll)]> "$m invoke 23;break"
    bind $w <[repctrl $ayviewshortcuts(Align2)]> "$m invoke 24;break"

    if { ([winfo toplevel $w] == $w) || $ayprefs(BindInternalViews) } {
	bind $w <[repctrl $ayviewshortcuts(Local)]> "viewCyleMMode $w;break"
    }

    bind $w <[repctrl $ayviewshortcuts(Local2)]> "viewCycleMMode $w;break"

    # view actions
    bind $w <[repctrl $ayviewshortcuts(RotL)]>\
   "$w.f3D.togl setconf -undokb ViewRotX -drotx $::ayprefs(KbdRot) -droty 0.0;break"
    bind $w <[repctrl $ayviewshortcuts(RotR)]>\
   "$w.f3D.togl setconf -undokb ViewRotX -drotx -$::ayprefs(KbdRot) -droty 0.0;break"
    bind $w <[repctrl $ayviewshortcuts(RotU)]>\
   "$w.f3D.togl setconf -undokb ViewRotY -droty $::ayprefs(KbdRot) -drotx 0.0;break"
    bind $w <[repctrl $ayviewshortcuts(RotD)]>\
   "$w.f3D.togl setconf -undokb ViewRotY -droty -$::ayprefs(KbdRot) -drotx 0.0;break"

    bind $w <[repctrl $ayviewshortcuts(ZoomI)]>\
	"$w.f3D.togl setconf -undokb ViewZoomIn -dzoom $::ayprefs(KbdZoomI);break"

    bind $w <[repctrl $ayviewshortcuts(ZoomO)]>\
	"$w.f3D.togl setconf -undokb ViewZoomOut -dzoom $::ayprefs(KbdZoomO);break"

    bind $w <[repctrl $ayviewshortcuts(ZoomI2)]>\
	"$w.f3D.togl setconf -undokb ViewZoomIn -dzoom $::ayprefs(KbdZoomI);break"

    bind $w <[repctrl $ayviewshortcuts(ZoomO2)]>\
	"$w.f3D.togl setconf -undokb ViewZoomOut -dzoom $::ayprefs(KbdZoomO);break"

    bind $w <[repctrl $ayviewshortcuts(PanL)]>\
	"viewPan $w.f3D.togl 0;break"

    bind $w <[repctrl $ayviewshortcuts(PanR)]>\
	"viewPan $w.f3D.togl 1;break"

    bind $w <[repctrl $ayviewshortcuts(PanU)]>\
	"viewPan $w.f3D.togl 2;break"

    bind $w <[repctrl $ayviewshortcuts(PanD)]>\
	"viewPan $w.f3D.togl 3;break"

    bind $w <[repctrl $ayviewshortcuts(PanM)]>\
	"undo save PanMarkView; $w.f3D.togl setconf -panm 1; break"

    bind $w <[repctrl $ayviewshortcuts(SelAllP)]>\
	"selPnts -all; rV;break"
    bind $w <[repctrl $ayviewshortcuts(DeSelect)]>\
	"selPnts; rV;break"
    bind $w <[repctrl $ayviewshortcuts(InvSelP)]>\
	"invPnts; rV;break"

    bind $w <[repctrl $ayviewshortcuts(CollP)]>\
	"undo save Collapse; collMP; rV; break"
    bind $w <[repctrl $ayviewshortcuts(ExplP)]>\
	"undo save Explode; explMP; rV; break"

    bind $w <[repctrl $ayviewshortcuts(SnapGrid2D)]>\
	"actionSnapToGrid2D $w.f3D.togl;break"
    bind $w <[repctrl $ayviewshortcuts(SnapGrid3D)]>\
	"actionSnapToGrid3D $w.f3D.togl;break"

    bind $w <[repctrl $ayviewshortcuts(SnapGrid2DO)]>\
	"actionSnapToGrid2DO $w.f3D.togl;break"
    bind $w <[repctrl $ayviewshortcuts(SnapGrid3DO)]>\
	"actionSnapToGrid3DO $w.f3D.togl;break"


    bind $w <[repctrl $ayviewshortcuts(SnapMark)]>\
	"actionSnapToMark $w.f3D.togl;break"

    bind $w <[repctrl $ayviewshortcuts(SnapMarkO)]>\
	"actionSnapToMarkO $w.f3D.togl;break"

    bind $w <[repctrl $ayviewshortcuts(TagB)]>\
	"actionTagB $w.f3D.togl;break"

    set m $ay(toolsmenu)
    bind $w <[repctrl $ayviewshortcuts(Hide)]>\
	"$m invoke 12;break"
    bind $w <[repctrl $ayviewshortcuts(Show)]>\
	"$m invoke 14;break"

    bind $w <[repctrl $ayviewshortcuts(TypeUp)]> "viewCycleType $w -1;break"
    bind $w <[repctrl $ayviewshortcuts(TypeDown)]> "viewCycleType $w 1;break"

    bind $w <[repctrl $ayviewshortcuts(DMUp)]> "viewCycleDrawMode $w -1;break"
    bind $w <[repctrl $ayviewshortcuts(DMDown)]> "viewCycleDrawMode $w 1;break"

    bind $w <[repctrl $ayviewshortcuts(GridCycleUp)]>\
	"viewCycleGrid $w 1 1;break"
    bind $w <[repctrl $ayviewshortcuts(GridCycleDown)]>\
	"viewCycleGrid $w 1 -1;break"
    bind $w <[repctrl $ayviewshortcuts(GridUp)]> "viewCycleGrid $w 0 1;break"
    bind $w <[repctrl $ayviewshortcuts(GridDown)]> "viewCycleGrid $w 0 -1;break"

    bind $w <[repctrl $aymainshortcuts(Zap)]> zap
    bind $w <Map> unzap

    # object selection
    bind $w <[repctrl $ayviewshortcuts(OSUp)]>\
	"selNPFL 1; after idle \"focus %W\""
    bind $w <[repctrl $ayviewshortcuts(OSDown)]>\
	"selNPFL 0; after idle \"focus %W\""
    bind $w <[repctrl $ayviewshortcuts(OSSUp)]>\
	"selAdd 0; after idle \"focus %W\""
    bind $w <[repctrl $ayviewshortcuts(OSSDown)]>\
	"selAdd 1; after idle \"focus %W\""
    # scene hierarchy navigation
    bind $w <[repctrl $ayviewshortcuts(OSLeft)]> {
	set oldfocus [focus]
	if { $ay(lb) == 0 } {
	    # Tree
	    set widget $ay(tree)
	} else {
	    # Listbox
	    set widget $ay(olb)
	}
	focus $widget
	event generate $widget <Left>
	after idle "focus $oldfocus"
    }

    bind $w <[repctrl $ayviewshortcuts(OSRight)]> {
	set oldfocus [focus]
	if { $ay(lb) == 0 } {
	    # Tree
	    set widget $ay(tree)
	} else {
	    # Listbox
	    set widget $ay(olb)
	}
	focus $widget
	event generate $widget <Right>
	after idle "focus $oldfocus"
    }

    # reset view camera
    bind $w <[repctrl $ayviewshortcuts(Reset)]> {
	if { [string first ".view" %W] != 0 } {
	    set w %W
	} else {
	    set w [winfo toplevel %W]
	}
	viewReset $w
    }

    bind $w <[repctrl $ayviewshortcuts(TransO)]> {
	if { [string first ".view" %W] != 0 } {
	    set w %W
	} else {
	    set w [winfo toplevel %W]
	}
	$w.f3D.togl mc
	viewSetPMode $w 0
	break
    }
    bind $w <Double-[repctrl $ayviewshortcuts(TransO)]> {
	foreach view $ay(views) {
	    ${view}.f3D.togl mc
	    viewSetPMode $view 0
	}
	break
    }

    bind $w <[repctrl $ayviewshortcuts(TransP)]> {
	if { [string first ".view" %W] != 0 } {
	    set w %W
	} else {
	    set w [winfo toplevel %W]
	}
	$w.f3D.togl mc
	viewSetPMode $w 1
	break
    }
    bind $w <Double-[repctrl $ayviewshortcuts(TransP)]> {
	foreach view $ay(views) {
	    ${view}.f3D.togl mc
	    viewSetPMode $view 1
	}
	break
    }

    bind $w <[repctrl $ayviewshortcuts(ResetAllW)]>\
	"actionResetAllWP $w.f3D.togl"

    # bind function keys
    shortcut_fkeys $w

    # on Aqua, in external views, <Command-q> quits the application
    if { ($AYWITHAQUA == 1) && ([winfo toplevel $w] == $w) } {
	bind $w <Command-q> exit
    }

 return;
}
# shortcut_view


# shortcut_viewalt:
#
proc shortcut_viewalt { ww } {
    global ay tcl_version
    if { $ay(ws) == "Windows" && $tcl_version > 8.4 } {
	::tk::MenuUnpost {}
    }
    if { [string first ".view" $ww] != 0 } {
	set w $ww.f3D.togl
    } else {
	set w [winfo toplevel $ww].f3D.togl
    }
    $w mc
    $w configure -cursor exchange
    set ay(vaoldfocus) [focus]
    update idletasks

 return;
}
# shortcut_viewalt


# shortcut_viewactions:
# Setup action-keybindings for a 3D-View.
proc shortcut_viewactions { w } {
 global ay ayviewshortcuts

    # Rotate by mouse
    bind all <KeyRelease-$ayviewshortcuts(RotModKey)> "update idletasks;"

    # as the Alt modifier is repeating on Windows we must immediately
    # disable further events; otherwise our Release-binding below
    # does not work correctly, leaving the focus nowhere after simply
    # pressing/releasing <Alt> and then the wheel would not work anymore
    # for zooming the view (until <B1> is used to 'repair' the focus)
    bind $w <KeyPress-$ayviewshortcuts(RotModKey)> {
	bind %W <KeyPress-$ayviewshortcuts(RotModKey)> ""
	shortcut_viewalt %W
    }
    bind $w <KeyRelease-$ayviewshortcuts(RotModKey)> {
	bind %W <KeyPress-$ayviewshortcuts(RotModKey)> {
	    bind %W <KeyPress-$ayviewshortcuts(RotModKey)> ""
	    shortcut_viewalt %W
	}
	# must not do anything to cursor or focus if the Alt keypress
	# served to "tab away" to another application, otherwise our
	# taskbar icon may get active, annoying the user...
	after 100 {
	    if { [focus] != "" } {
		if { [string first ".view" %W] != 0 } {
		    set w %W.f3D.togl
		} else {
		    set w [winfo toplevel %W].f3D.togl
		}
		$w mc
		$w configure -cursor {}

		if { $ay(vaoldfocus) != "" } {
		    if { [winfo exists $ay(vaoldfocus)] } {
			focus -force $ay(vaoldfocus)
		    } else {
			set ay(vaoldfocus) ""
		    }
		}
	    }
	}
    }
    bind $w <Enter> {+
	bind %W <KeyPress-$ayviewshortcuts(RotModKey)> {
	    bind %W <KeyPress-$ayviewshortcuts(RotModKey)> ""
	    shortcut_viewalt %W
	}
	if { [string first ".view" %W] != 0 } {
	    set w %W.f3D.togl
	} else {
	    set w [winfo toplevel %W].f3D.togl
	}
	$w configure -cursor {}
    }

    set i $ayviewshortcuts(RotButton)
    bind $w.f3D.togl <${ayviewshortcuts(RotMod)}-ButtonPress-${i}> {
	%W mc
	if { $ay(cVUndo) } {
	    undo save RotateView
	}
	%W setconf -a 1
	set oldx %x
	set oldy %y
	set ay(motion) 0
    }
    bind $w.f3D.togl <${ayviewshortcuts(RotMod)}-ButtonRelease-${i}> {
	%W setconf -a 0
	focus %W
	if { $ay(cVUndo) && ! $ay(motion) } {
	    undo rewind silent
	}
	break
    }
    bind $w.f3D.togl <${ayviewshortcuts(RotMod)}-B${i}-Motion> {
	%W setconf -drotx [expr ($oldx - %x)] -droty [expr ($oldy - %y)]
	set oldx %x
	set oldy %y
	set ay(motion) 1
	break
    }

    # Zoom/Pan into a region by mouse
    bind $w <KeyPress-$ayviewshortcuts(ZoomRModKey)> {
	if { [string first ".view" %W] != 0 } {
	    set w %W.f3D.togl
	} else {
	    set w [winfo toplevel %W].f3D.togl
	}
	$w configure -cursor sizing
    }

    bind $w <KeyRelease-$ayviewshortcuts(ZoomRModKey)> {
	if { [string first ".view" %W] != 0 } {
	    set w %W.f3D.togl
	} else {
	    set w [winfo toplevel %W].f3D.togl
	}
	$w configure -cursor {}
    }

    set i $ayviewshortcuts(ZoomRButton)
    bind $w.f3D.togl <${ayviewshortcuts(ZoomRMod)}-ButtonPress-${i}> {
	set oldx %x
	set oldy %y
    }
    bind $w.f3D.togl <${ayviewshortcuts(ZoomRMod)}-ButtonRelease-${i}> {
	if { $oldx != %x && $oldy != %y } {
	    %W mc
	    if { $ay(cVUndo) } {
		undo save ZoomRView
	    }
	    %W setconf -zrect 1 -rect $oldx $oldy %x %y 0
	    %W redraw
	    focus %W
	}
	break
    }
    bind $w.f3D.togl <${ayviewshortcuts(ZoomRMod)}-B${i}-Motion> {
	%W setconf -rect $oldx $oldy %x %y 1
    }

    # Zoom by mouse
    set i $ayviewshortcuts(ZoomVButton)
    bind $w.f3D.togl <ButtonPress-${i}> {
	%W mc
	if { $ay(cVUndo) } {
	    undo save ZoomView
	}
	%W setconf -a 1
	%W zoomvac -start %y
	set ay(motion) 0
    }
    bind $w.f3D.togl <B${i}-Motion> {
	%W zoomvac -winy %y
	set ay(motion) 1
    }
    bind $w.f3D.togl <ButtonRelease-${i}> {
	%W setconf -a 0
	if { $ay(cVUndo) && ! $ay(motion) } {
	    undo rewind silent
	}
	focus %W
    }

    # Pan by mouse
    set i $ayviewshortcuts(MoveVButton)
    bind $w.f3D.togl <ButtonPress-${i}> {
	%W mc
	if { $ay(cVUndo) } {
	    undo save MoveView
	}
	%W setconf -a 1
	%W movevac -start %x %y
	set ay(motion) 0
    }
    bind $w.f3D.togl <B${i}-Motion> {
	%W movevac -winxy %x %y
	set ay(motion) 1
    }
    bind $w.f3D.togl <ButtonRelease-${i}> {
	%W setconf -a 0
	if { $ay(cVUndo) && ! $ay(motion) } {
	    undo rewind silent
	}
	focus %W
    }

    # Zoom by mouse wheel
    # XXXX ToDo: should we bind B4/B5 on Windows/Aqua?
    bind $w.f3D.togl <ButtonPress-4> {viewZoom %W %D;break}

    bind $w.f3D.togl <ButtonPress-5> {viewZoom %W -%D;break}

    if { $ay(ws) == "Windows" || $ay(ws) == "Aqua" } {
	if { [string first ".view" $w] != 0 } {
	    # internal view
	    if { $ay(ws) == "Windows" } {
		bind $w <MouseWheel> {viewZoom %W.f3D.togl %D;break}
	    } else {
		# Aqua
		bind $w.f3D.togl <MouseWheel> {viewZoom %W %D;break}
	    }
	} else {
	    # external view
	    bind $w <MouseWheel> {viewZoom %W %D;break}
	}
	# if internal or external
    }
    # if Windows or Aqua

    # Roll by mouse wheel
    # XXXX ToDo: should we bind B4/B5 on Windows/Aqua?
    bind $w.f3D.togl <${ayviewshortcuts(RollMod)}-ButtonPress-4> \
     {viewRoll %W %D;break}

    bind $w.f3D.togl <${ayviewshortcuts(RollMod)}-ButtonPress-5> \
     {viewRoll %W -%D;break}

    if { $ay(ws) == "Windows" || $ay(ws) == "Aqua" } {
	if { [string first ".view" $w] != 0 } {
	    # internal view
	    if { $ay(ws) == "Windows" } {
		bind $w <${ayviewshortcuts(RollMod)}-MouseWheel> \
		    {viewRoll %W.f3D.togl %D;break}
	    } else {
		# Aqua
		bind $w.f3D.togl <${ayviewshortcuts(RollMod)}-MouseWheel> \
		    {viewRoll %W %D;break}
	    }
	} else {
	    # external view
	    bind $w <${ayviewshortcuts(RollMod)}-MouseWheel> \
		{viewRoll %W %D;break}
	}
	# if internal or external
    }
    # if Windows or Aqua

    # Start modelling actions by keyboard
    bind $w <$ayviewshortcuts(Break)> "actionClear $w.f3D.togl"
    bind $w <$ayviewshortcuts(MoveV)> "actionMoveView $w.f3D.togl"
    bind $w <$ayviewshortcuts(MoveZV)> "actionMoveZView $w.f3D.togl"
    bind $w <$ayviewshortcuts(RotV)> "actionRotView $w.f3D.togl"
    bind $w <$ayviewshortcuts(ZoomV)> "actionZoomView $w.f3D.togl"

    bind $w <$ayviewshortcuts(MoveO)> "actionMoveOb $w.f3D.togl"
    bind $w <$ayviewshortcuts(RotO)> "actionRotOb $w.f3D.togl"
    bind $w <$ayviewshortcuts(Scal2)> "actionSc2DOb $w.f3D.togl"
    bind $w <$ayviewshortcuts(Scal3)> "actionSc3DOb $w.f3D.togl"
    bind $w <$ayviewshortcuts(Stretch)> "actionStr2DOb $w.f3D.togl"
    bind $w <$ayviewshortcuts(Edit)> "actionEditP $w.f3D.togl"
    bind $w <$ayviewshortcuts(EditN)> "actionEditNumP $w.f3D.togl"
    bind $w <$ayviewshortcuts(WeightE)> "actionEditWP $w.f3D.togl"
    bind $w <$ayviewshortcuts(WeightR)> "actionResetWP $w.f3D.togl"
    bind $w <$ayviewshortcuts(About)> "actionSetMark $w.f3D.togl"

    bind $w <$ayviewshortcuts(Select)> "actionTagP $w.f3D.togl"
    bind $w <$ayviewshortcuts(InsertP)> "actionInsertP $w.f3D.togl"
    bind $w <$ayviewshortcuts(DeleteP)> "actionDeleteP $w.f3D.togl"
    bind $w <$ayviewshortcuts(FindU)> "actionFindU $w.f3D.togl"
    bind $w <$ayviewshortcuts(FindUV)> "actionFindUV $w.f3D.togl"
    bind $w <$ayviewshortcuts(SelBnd)> "actionSelBnd $w.f3D.togl"
    bind $w <$ayviewshortcuts(SelBndC)> "actionSelBnd $w.f3D.togl 1"
    bind $w <$ayviewshortcuts(SplitNC)> "actionSplitNC $w.f3D.togl"

    bind $w <$ayviewshortcuts(IncMultP)> "actionIncMultP $w.f3D.togl"
    bind $w <$ayviewshortcuts(DecMultP)> "actionDecMultP $w.f3D.togl"

    bind $w <$ayviewshortcuts(OpenC)> {
	undo save OpenC
	openC
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(CloseC)> {
	undo save CloseC
	closeC
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(RefineC)> {
	if { [string first "FindU" [viewGetMAIcon %W.f3D.togl]] != -1 } {
	    undo save RefineKNC
	    refineknNC
	} else {
	    undo save RefineC
	    refineC
	}
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(CoarsenC)> {
	undo save CoarsenC
	coarsenC
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(RevertC)> {
	undo save RevertC
	revertC
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(ShiftFC)> {
	undo save ShiftFC
	shiftC 1
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(ShiftBC)> {
	undo save ShiftBC
	shiftC -1
	plb_update; rV
	after idle "focus %W"
    }
    bind $w <$ayviewshortcuts(Pick)> "actionPick $w.f3D.togl"

    bind $w.f3D.togl <[repctrl $ayviewshortcuts(SetMark)]>\
     "$w.f3D.togl mc; $w.f3D.togl setconf -mark %x %y"

    bind $w <[repctrl $ayviewshortcuts(ClearMark)]>\
     "$w.f3D.togl mc; $w.f3D.togl setconf -mark n; puts \"Mark cleared.\""

    # allow plugins to define their own actions
    foreach customkey $ay(customkeys) {
	catch { bind $w [lindex $customkey 0] [lindex $customkey 1] }
    }

 return;
}
# shortcut_viewactions


#shortcut_show:
# display all current shortcuts in a separate top level window
proc shortcut_show { } {
global ay ayprefs aymainshortcuts ayviewshortcuts tcl_platform

set w .ayscw
set t "Ayam Shortcuts"
winDialog $w $t

if { $ayprefs(FixDialogTitles) == 1 } {
    pack [frame $w.fl] -in $w -side top
    pack [label $w.fl.l -text $t] -in $w.fl -side left -fill x -expand yes
}

frame $w.ftext

frame $w.fbutton
pack $w.fbutton -in $w -side bottom
button $w.fbutton.b -text "Close" -pady $ay(pady) -command "destroy $w"
if { $tcl_platform(platform) == "windows" } {
    pack [frame $w.fbutton.f -height 2] -in $w.fbutton -side top
}
pack $w.fbutton.b -in $w.fbutton -side top

# Esc-key && close via window decoration == Close button
bind $w <Escape> "$w.fbutton.b invoke"
wm protocol $w WM_DELETE_WINDOW "$w.fbutton.b invoke"

pack $w.ftext -in $w -side top -expand yes -fill both

set font fixed

if { $tcl_platform(platform) == "windows" } {
    set font fixedsys
}

# IRIXens "fixed" is too wide...
if { [string first "IRIX" $tcl_platform(os)] != -1 } {
    set font 9x15
}

# Also MacOSX Aqua does not deliver a proper mono-spaced "fixed"...
if { $ay(ws) == "Aqua"  } {
    set font {Courier 12}
}

if { $::tcl_version > "8.4" } {
    set font "monospace"
    if { $ay(ws) == "Windows"  } {
	set font "TkFixedFont"
    }
} else {
    set gs $ay(GuiScale)
    if { $gs > 1.0 } {
	set font "Courier [expr round(10 * $gs)]"
	if { $gs > 1.4 } {
	    append font " bold"
	}
    }
}

text $w.ftext.text -yscrollcommand "$w.ftext.sbar set" \
	-setgrid 1 -font $font -height 20 -width 40 -wrap none
# -tabs {32c left}
if { $ay(ws) == "Aqua"  } {
    $w.ftext conf -relief sunken -bd 2
}
scrollbar $w.ftext.sbar -takefocus 0 -command "$w.ftext.text yview"

pack $w.ftext.sbar -in $w.ftext -side right -fill y
pack $w.ftext.text -in $w.ftext -side left -fill both -expand yes

# strip KeyRelease/KeyPress elements, also map some inexplicable keysyms
set mappings [shortcut_getmappings]

foreach elem [array names ayviewshortcuts] {
    eval set sc1 \$ayviewshortcuts(${elem})
    set sc2 ""
    set indexr [string first "KeyRelease-" $sc1]
    set indexp [string first "KeyPress-" $sc1]
    if { ($indexr > -1) || ($indexp) > 0 } {
	if { $indexr > -1 } {
	    if { $indexr > 0 } {
		set sc2 [string range $sc1 0 [expr $indexr - 1]]
	    }
	    append sc2 [string range $sc1 [expr $indexr + 11] end]
	}

	if { $indexp > -1 } {
	    if { $indexp > 0 } {
		set sc2 [string range $sc1 0 [expr $indexp - 1]]
	    }
	    set sc2 [string range $sc1 [expr $indexp + 9] end]
	}
    } else {
	set sc2 $sc1
    }

    set sc2 [string map $mappings $sc2]

    set svsc(${elem}) $sc2
}
# foreach

$w.ftext.text insert end "Modelling Scope:

 Objects (all views)  <$svsc(TransO)> (<$svsc(TransO)$svsc(TransO)>)
 Points (all views)   <$svsc(TransP)> (<$svsc(TransP)$svsc(TransP)>)

Modelling Actions:

 Move                 <$svsc(MoveO)>
  restrict X, Y, Z:   <$svsc(MoveO)$svsc(RestrictX)>, <$svsc(MoveO)$svsc(RestrictY)>, <$svsc(MoveO)$svsc(RestrictZ)>

 Rotate               <$svsc(RotO)>
  about Mark          <$svsc(RotO)$svsc(About)>
  about Objs Center   <$svsc(RotO)$svsc(CenterO)>
  about Pnts Center   <$svsc(RotO)$svsc(CenterPB)>

 Scale (3D)           <$svsc(Scal3)>
 Scale (2D)           <$svsc(Scal2)>
 Stretch (2D)         <$svsc(Stretch)>
 Scale (1D,X)         <$svsc(Scal2)$svsc(RestrictX)>
 Scale (1D,Y)         <$svsc(Scal2)$svsc(RestrictY)>
 Scale (1D,Z)         <$svsc(Scal2)$svsc(RestrictZ)>
 Scale 2D
  about Mark          <$svsc(Scal2)$svsc(About)>
  about Objs Center   <$svsc(Scal2)$svsc(CenterO)>
  about Pnts Center   <$svsc(Scal2)$svsc(CenterPB)>
 Similar for other scale actions...

 Set Mark             <$svsc(About)>
  to Objs Center      <$svsc(About)$svsc(CenterO)>
  to Pnts Center      <$svsc(About)$svsc(CenterPB)>

 Edit Points Direct   <$svsc(Edit)>
 Edit Points Numeric  <$svsc(EditN)>
 Edit Weight          <$svsc(WeightE)>
 Reset Weights        <$svsc(WeightR)>

 Insert Points        <$svsc(InsertP)>
 Delete Points        <$svsc(DeleteP)>

 Collapse Selected    <$svsc(CollP)>
 Explode Selected     <$svsc(ExplP)>
 Snap to Grid 2D      <$svsc(SnapGrid2D)>
 Snap to Grid 3D      <$svsc(SnapGrid3D)>
 Snap to Mark         <$svsc(SnapMark)>

 FindU                <$svsc(FindU)>
 FindUV               <$svsc(FindUV)>

 Open/Close           <$svsc(OpenC)> / <$svsc(CloseC)>
 Refine/Coarsen       <$svsc(RefineC)> / <$svsc(CoarsenC)>

 Revert/Split Curve   <$svsc(RevertC)> / <$svsc(SplitNC)>

Selection Actions:

 Select Objects       <$svsc(Pick)>
 Select Points        <$svsc(Select)>
 Select All/No Points <$svsc(SelAllP)> / <$svsc(DeSelect)>
 Invert PointSel      <$svsc(InvSelP)>

 Select Boundary/Trim <$svsc(SelBnd)> / <$svsc(SelBndC)>

View Actions:

 Move View            <$svsc(MoveV)>
 Move View
 (along Z)            <$svsc(MoveZV)>
 MoveViewAnytime      <Mouse-$svsc(MoveVButton)>

 Rotate View          <$svsc(RotV)>
 RotateViewAnytime    <$svsc(RotModKey)+Mouse-1>

 Rotate View (left)   <$svsc(RotL)>
 Rotate View (right)  <$svsc(RotR)>
 Rotate View (up)     <$svsc(RotU)>
 Rotate View (down)   <$svsc(RotD)>

 Zoom View            <$svsc(ZoomV)>
 Zoom in View         <$svsc(ZoomI)>
 Zoom out View        <$svsc(ZoomO)>
 ZoomViewAnytime      <Mouse-$svsc(ZoomVButton)>

 ZoomRegion           <$svsc(ZoomRModKey)+Mouse-1>
 ZoomToObject         <$svsc(ZoomTO2)> / <$svsc(ZoomTO)>
 ZoomToAll            <$svsc(ZoomAll)>

 Cycle View Type      <$svsc(TypeUp)> / <$svsc(TypeDown)>

 Cycle Draw Mode      <$svsc(DMUp)> /
                        <$svsc(DMDown)>

Management:

 Break Action         <$svsc(Break)>

 Hide Objects         <$svsc(Hide)>
 Show Objects         <$svsc(Show)>

Function Keys:

 Adjust NURBS Sampling Rate  <${aymainshortcuts(SetSTL)}> / <${aymainshortcuts(SetSTP)}>
 Toggle Wire/NURBS           <${aymainshortcuts(SwNURBSWire)}>
 Toggle Lazy Notification    <${aymainshortcuts(SwLazyNotify)}>
 Update                      <${aymainshortcuts(Update)}>

 See the reference card in the docs
 folder for a printable and more
 complete version.
"

bind $w <Next> "$w.ftext.text yview scroll 1 pages"
bind $w <Prior> "$w.ftext.text yview scroll -1 pages"

bind $w.ftext.text <ButtonPress-4>\
 "$w.ftext.text yview scroll -4 units; break"

bind $w.ftext.text <ButtonPress-5>\
 "$w.ftext.text yview scroll 4 units; break"

if { $ay(ws) == "Windows" } {
    bind $w <MouseWheel> "\
	if { %D < 0.0 } {\
	    $w.ftext.text yview scroll 4 units\
	} else {\
	    $w.ftext.text yview scroll -4 units\
	}\
    "
}

# add a pinstripe background pattern, also prepare list of lines to underline
set filler "                                                        "
set i 0
set on 0
while { $i < [expr int([$w.ftext.text index end-1c]) -4] } {
    set line [$w.ftext.text get ${i}.0 ${i}.end]
    if { [string length $line] > 0 } {
	if { $on } {
	    # extend dark lines
	    $w.ftext.text insert ${i}.end $filler
	    update
	    # add line to range list for the graytag
	    lappend ranges ${i}.0
	    lappend ranges ${i}.end
	}
	set j [string first "<" $line]
	if { $j != -1 } {
	    # only lines with "<" toggle the pattern
	    if { $on } {
		set on 0
	    } else {
		set on 1
	    }
	} else {
	    # non-empty lines without "<" will be underlined
	    if { [string range $line 0 0] != " " } {
		lappend ulranges ${i}.0
		lappend ulranges ${i}.end
		set on 0
	    }
	}
    } else {
	# empty lines reset the pinstripe pattern
	set on 0
    }
    incr i
}
# while

eval $w.ftext.text tag add ultag $ulranges
$w.ftext.text tag configure ultag -underline 1

eval $w.ftext.text tag add graytag $ranges

# calculate new (darker) color from current background color
set oldbg [$w.ftext.text cget -background]
set r0 -1
if { [string first "\#" $oldbg] > -1 } {
    scan $oldbg "\#%2x%2x%2x" r0 g0 b0
} else {
    set rgb [winfo rgb . $oldbg]
    set r0 [expr [lindex $rgb 0] / 65535 * 256]
    set g0 [expr [lindex $rgb 1] / 65535 * 256]
    set b0 [expr [lindex $rgb 2] / 65535 * 256]
}
if { $r0 != -1 } {
    set r0 [expr $r0 - 10]
    set g0 [expr $g0 - 10]
    set b0 [expr $b0 - 10]
    set newbg [format "\#%2x%2x%2x" $r0 $g0 $b0]
    # set new color
    $w.ftext.text tag configure graytag -background $newbg
}

$w.ftext.text configure -state disabled

winRestoreOrCenter $w $t

 return;
}
# shortcut_show


# shortcut_addescescbinding:
#
proc shortcut_addescescbinding { w } {
    bind $w <Escape> "\
        bind $w <Escape> \"\";
        resetFocus;
	cS;
        \$ay(plb) selection clear 0 end;
        plb_update;
        if \{ \$ay(lb) == 0 \} \{ tree_gotop \} else \{ goTop \};
        rV;"
    after 1000 "bind $w <Escape> \{shortcut_addescescbinding $w\}"

 return;
}
# shortcut_addescescbinding


# shortcut_callcshelp:
#  helper for shortcut_addcshelp below
#  actually calls the web browser
proc shortcut_callcshelp { base tag } {
    global ayprefs
    openUrl [concatUrls ${ayprefs(Docs)} ${base}\#${tag}]
 return;
}
# shortcut_callcshelp


# shortcut_addcshelp:
#  add context sensitive help binding to window <w>
#  base: html file, e.g. ayam-5.html, tag: html tag, e.g. tesst
#  => http://ayam.sourceforge.net/docs/ayam-5.html#tesst
proc shortcut_addcshelp { w base tag } {
    global aymainshortcuts
    bind $w <[repctrl $aymainshortcuts(Help)]> "shortcut_callcshelp $base $tag"
 return;
}
# shortcut_addcshelp


# shortcut_addviewbinding:
#  add a keyboard shortcut to all views
proc shortcut_addviewbinding { k b } {
    global ay

    # arrange to bind to future views
    set add "bind \$w "
    append add $k " " \"$b\"
    addToProc shortcut_view $add

    # bind to current views
    foreach v $ay(views) {
	catch {bind $v $k $b}
    }
 return;
}
# shortcut_addviewbinding


proc shortcut_getmappings { } {
    lappend mappings bar \|
    lappend mappings parenright \)
    lappend mappings parenleft \(
    lappend mappings plus \+
    lappend mappings minus \-
    lappend mappings underscore \_
    lappend mappings asterisk \*
    lappend mappings slash \/
    lappend mappings numbersign \#
    lappend mappings apostrophe \'
    lappend mappings quoteright \'
    lappend mappings period \.
    lappend mappings exclam \!
    lappend mappings less \<
    lappend mappings greater \>
    lappend mappings question \?

 return $mappings;
}
# shortcut_getmappings
