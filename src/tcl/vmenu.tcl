# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2005 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# vmenu.tcl - the view menu

proc vmenu_open { w } {
global ay ayprefs aymainshortcuts AYWITHAQUA

if { (! $AYWITHAQUA) || ([winfo toplevel $w] != $w) } {
    set menubar 0
} else {
    set menubar 1
}

# View Menu
if { $menubar } {
    set mb [menu $w.menubar -tearoff 0 -type menubar]
    $w configure -menu $mb
    set m [menu $mb.mview -tearoff 0]
    $mb add cascade -label "View" -menu $m

    if { $AYWITHAQUA } {
	# correct application menu (about entry)
	menu $w.menubar.apple
	$w.menubar add cascade -menu $w.menubar.apple
	$w.menubar.apple add command -label "About Ayam" -command "aboutAyam"
    }
} else {
    menubutton $w.fMenu.v -text "View" -menu $w.fMenu.v.m
    if { ! $AYWITHAQUA } {
	$w.fMenu.v configure -padx 3
    }
    if { [winfo toplevel $w] == $w } {
	$w.fMenu.v configure -underline 0
    }
    set m [menu $w.fMenu.v.m -tearoff 0]
}

$m add command -label "Quick Render" -command "viewRender $w 1;\
	\$ay(currentView) mc"

$m add command -label "Render" -command "viewRender $w 0;\
	\$ay(currentView) mc"

$m add command -label "Render To File" -command "viewRender $w 2;\
	\$ay(currentView) mc"

$m add command -label "Export RIB" -command "\
    io_exportRIB $w"

$m add separator

$m add command -label "Redraw" -command "\
	global ay;\
	$w.f3D.togl mc;\
	$w.f3D.togl reshape;\
	$w.f3D.togl redraw;\
	\$ay(currentView) mc"

$m add command -label "Reset" -command "viewReset $w"


global AYENABLEPPREV
if { $AYENABLEPPREV == 1 } {
    $m add separator

    $m add command -label "Open PPrev" -command {
	set togl $ay(currentView)
	set w [winfo toplevel $togl]
	$togl setconf -pprev 1
    }

    $m add command -label "Close PPrev" -command {
	set togl $ay(currentView)
	set w [winfo toplevel $togl]
	$togl setconf -pprev 0
    }
}
# if

$m add separator

$m add command -label "Create All ShadowMaps" -command "io_RenderSM $w 1"
$m add command -label "Create ShadowMap" -command "io_RenderSM $w 0"

if { [string first ".view" $w] == 0 } {

    $m add separator

    # "after 100" because on Windows the <Enter>-binding fires when the menu
    # is closed and runs parallel to "viewClose" resulting in an error
    $m add command -label "Close" -command \
    "after 100 \{viewUnBind $w;viewClose $w;global ay;set ay(ul) root:0;uS\}"
}

# Type Menu
if { $menubar } {
    set m [menu $mb.mtype -tearoff 0]
    $mb add cascade -label "Type" -menu $m
} else {
    menubutton $w.fMenu.t -text "Type" -menu $w.fMenu.t.m
    if { ! $AYWITHAQUA } {
	$w.fMenu.t configure -padx 3
    }
    if { [winfo toplevel $w] == $w } {
	$w.fMenu.t configure -underline 0
    }
    set m [menu $w.fMenu.t.m -tearoff 0]
}

$m add command -label "Front" -command "viewSetType $w 0" -underline 0
$m add command -label "Side" -command "viewSetType $w 1" -underline 0
$m add command -label "Top" -command "viewSetType $w 2" -underline 0
$m add separator
$m add command -label "Perspective" -command "viewSetType $w 3" -underline 0
$m add separator
$m add command -label "Trim" -command "viewSetType $w 4" -underline 1

# Configure Menu
if { $menubar } {
    set m [menu $mb.mconf -tearoff 0]
    $mb add cascade -label "Configure" -menu $m
} else {
    menubutton $w.fMenu.c -text "Configure" -menu $w.fMenu.c.m
    if { ! $AYWITHAQUA } {
	$w.fMenu.c configure -padx 3
    }
    if { [winfo toplevel $w] == $w } {
	$w.fMenu.c configure -underline 0
    }
    set m [menu $w.fMenu.c.m -tearoff 0]
}

set confm $m

$m add check -label "Automatic Redraw" -variable ay(cVRedraw) -command "\
	global ay;\
	$w.f3D.togl setconf -draw \$ay(cVRedraw);\
	\$ay(currentView) mc"

# drawing mode sub menu
set cm [menu $m.dmode -tearoff 0]
$m add cascade -label "Drawing Mode" -menu $cm

$cm add radio -label "Draw" -variable ay(cVDMode) -value 0 -command "\
	global ay;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 0;\
	\$ay(currentView) mc"

$cm add radio -label "Shade" -variable ay(cVDMode) -value 1 -command "\
	global ay;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 1;\
	\$ay(currentView) mc"

$cm add radio -label "ShadeAndDraw" -variable ay(cVDMode) -value 2 -command "\
	global ay;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 2;\
	\$ay(currentView) mc"
$cm add radio -label "HiddenWire" -variable ay(cVDMode) -value 3 -command "\
	global ay;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 3;\
	\$ay(currentView) mc"

# modelling mode sub menu
set cm [menu $m.mmode -tearoff 0]
$m add cascade -label "Modelling Mode" -menu $cm

$cm add radio -label "Global" -variable ay(cVMMode) -value 0 -command "\
        global ay;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
        viewSetMModeIcon $w \$ay(cVMMode);\
	\$ay(currentView) mc"

$cm add radio -label "Local (Level)" -variable ay(cVMMode) -value 1 -command "\
        global ay;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
        viewSetMModeIcon $w \$ay(cVMMode);\
	\$ay(currentView) mc"

$cm add radio -label "Local (Object)" -variable ay(cVMMode) -value 2\
    -command "\
        global ay;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
        viewSetMModeIcon $w \$ay(cVMMode);\
	\$ay(currentView) mc"

$m add check -label "Draw Selection only" -variable ay(cVDrawSel) -command "\
	global ay;\
	$w.f3D.togl setconf -dsel \$ay(cVDrawSel);\
	\$ay(currentView) mc"

$m add check -label "Draw Level only" -variable ay(cVDrawLevel) -command "\
	global ay;\
	$w.f3D.togl setconf -dlev \$ay(cVDrawLevel);\
	\$ay(currentView) mc"

$m add check -label "Draw Object CS" -variable ay(cVDrawOCS) -command "\
	global ay;\
	$w.f3D.togl setconf -docs \$ay(cVDrawOCS);\
	\$ay(currentView) mc"

$m add check -label "AntiAlias Lines" -variable ay(cVAALines) -command "\
	global ay;\
	$w.f3D.togl setconf -doaal \$ay(cVAALines);\
	\$ay(currentView) mc"

$m add separator
$m add check -label "Draw BGImage" -variable ay(cVDrawBG) -command "\
	global ay;\
	$w.f3D.togl setconf -dbg \$ay(cVDrawBG);\
	\$ay(currentView) mc"

$m add command -label "Set BGImage" -command "viewSetBGImage $w.f3D.togl"

$m add separator
$m add check -label "Draw Grid" -variable ay(cVDrawGrid) -command "\
	global ay;\
	$w.f3D.togl setconf -drawg \$ay(cVDrawGrid);\
	\$ay(currentView) mc"

$m add check -label "Use Grid" -variable ay(cVUseGrid) -command "\
	global ay;\
	$w.f3D.togl setconf -ugrid \$ay(cVUseGrid);\
	\$ay(currentView) mc"

$m add command -label "Set GridSize" -command "viewSetGrid $w.f3D.togl"

$m add separator
$m add command -label "Half Size" -command "\
    global ay;\
    set neww \[expr (\[winfo reqwidth $w.f3D.togl\] / 2)\];\
    set newh \[expr (\[winfo reqheight $w.f3D.togl\] / 2)\];\
    wm geometry $w \"\";\
    $w.f3D.togl mc;\
    $w.f3D.togl configure -width \$neww -height \$newh;\
    after 500 \"$w.f3D.togl mc; $w.f3D.togl setconf -dfromx 0.0\";\
    \$ay(currentView) mc"

$m add command -label "Double Size" -command "\
    global ay;\
    set neww \[expr (\[winfo reqwidth $w.f3D.togl\] * 2)\];\
    set newh \[expr (\[winfo reqheight $w.f3D.togl\] * 2)\];\
    wm geometry $w \"\";\
    $w.f3D.togl mc;\
    $w.f3D.togl configure -width \$neww -height \$newh;\
    after 500 \"$w.f3D.togl mc; $w.f3D.togl setconf -dfromx 0.0\";\
    \$ay(currentView) mc"

if { ([winfo toplevel $w] != $w) } {
    $m entryconfigure 15 -state disabled
    $m entryconfigure 16 -state disabled
}

$m add separator

$m add command -label "From Camera" -command "\
	global ay;\
	undo save FromCamera;\
	$w.f3D.togl mc; $w.f3D.togl fromcam; \$ay(currentView) mc"

$m add command -label "To Camera" -command "\
	global ay;\
	undo save ToCamera;\
	$w.f3D.togl mc; $w.f3D.togl tocam; \$ay(currentView) mc"

$m add command -label "Set FOV" -command "viewSetFOV $w.f3D.togl"

$m add separator
$m add command -label "Zoom to Object" -command "\
	global ay;\
	undo save ZoomToObj;\
	$w.f3D.togl mc; $w.f3D.togl zoomob; \$ay(currentView) mc"

$m add command -label "Zoom to All" -command "\
	global ay;\
	undo save ZoomToAll;\
	$w.f3D.togl mc; $w.f3D.togl zoomob -all; \$ay(currentView) mc"

$m add command -label "Align to Object" -command "\
	global ay;\
	undo save AlignToObj;\
	$w.f3D.togl mc; $w.f3D.togl align; \$ay(currentView) mc"


# Modelling Action Menu
if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
    menubutton $w.fMenu.a -image ay_EmptyG_img -menu $w.fMenu.a.m\
	-padx 0 -pady 0 -borderwidth 0
    set m [menu $w.fMenu.a.m -tearoff 0]
    bind $w.fMenu.a <ButtonPress-1>\
	"+tk_menuSetFocus $w; catch \{destroy %W.balloon\}"
    bind $w.fMenu.a <ButtonPress-3> vmenu_configureactionmenu
} else {
    set m [menu $mb.ma -tearoff 0]
    $mb add cascade -label Action -menu $m
    bind $mb <ButtonPress-3> vmenu_configureactionmenu
}

vmenu_fillactionmenu $w

# Modelling Mode Menu
if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
    menubutton $w.fMenu.mm -image ay_MMGlob_img -menu $w.fMenu.mm.m\
	    -padx 0 -pady 0 -borderwidth 0
    global ayviewshortcuts aymainshortcuts
    set lmk $ayviewshortcuts(Local2)
    balloon_set $w.fMenu.mm [string map [list <lmk> <$lmk>] [ms vmenu2]]
    set m [menu $w.fMenu.mm.m -tearoff 0]
    bind $w.fMenu.mm <ButtonPress-1>\
	"+tk_menuSetFocus $w; catch \{destroy %W.balloon\}"

    bind $w.fMenu.mm <ButtonPress-$aymainshortcuts(CMButton)>\
	"viewToggleModellingScope %W 2"
} else {
    set m [menu $mb.mm -tearoff 0]
    $mb add cascade -label Global -menu $m
}

$m add command -image ay_MMGlob_img -hidemargin 1 -command "\
        global ay; set ay(cVMMode) 0;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
	viewSetType $w \$ay(cVType) 1;\
	viewSetMModeIcon $w 0;\
	\$ay(currentView) mc"

$m add command -image ay_MMLocLev_img -hidemargin 1 -command "\
        global ay; set ay(cVMMode) 1;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
        $w.f3D.togl align;\
	viewSetMModeIcon $w 1;\
	\$ay(currentView) mc"

$m add command -image ay_MMLocObj_img -hidemargin 1 -command "\
        global ay; set ay(cVMMode) 2;\
	$w.f3D.togl setconf -local \$ay(cVMMode);\
        $w.f3D.togl align;\
	viewSetMModeIcon $w 2;\
	\$ay(currentView) mc"

if { $AYWITHAQUA } {
    $m entryconfigure 0 -image {} -label "Global"
    $m entryconfigure 1 -image {} -label "Local (Level)"
    $m entryconfigure 2 -image {} -label "Local (Object)"
}


# Drawing Mode Menu
if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
    menubutton $w.fMenu.dm -image ay_DMDraw_img -menu $w.fMenu.dm.m\
	    -padx 0 -pady 0 -borderwidth 0
    global ayviewshortcuts
    set dmu $ayviewshortcuts(DMUp)
    set dmd $ayviewshortcuts(DMDown)
    balloon_set $w.fMenu.dm [string map [list <dmu> <$dmu> <dmd> <$dmd>]\
				 [ms vmenu3]]
    set m [menu $w.fMenu.dm.m -tearoff 0]
    bind $w.fMenu.dm <ButtonPress-1>\
	"+tk_menuSetFocus $w; catch \{destroy %W.balloon\}"
    set ay(dmodem) fMenu.dm.m
} else {
    set m [menu $mb.dm -tearoff 0]
    $mb add cascade -label Draw -menu $m
    set ay(dmodem) menubar.dm
}

$m add command -image ay_DMDraw_img -hidemargin 1 -command "\
        global ay; set ay(cVDMode) 0;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 0;\
	\$ay(currentView) mc"

$m add command -image ay_DMShade_img -hidemargin 1 -command "\
        global ay; set ay(cVDMode) 1;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 1;\
	\$ay(currentView) mc"

$m add command -image ay_DMShadeDraw_img -hidemargin 1 -command "\
        global ay; set ay(cVDMode) 2;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 2;\
	\$ay(currentView) mc"

$m add command -image ay_DMHiddenWire_img -hidemargin 1 -command "\
        global ay; set ay(cVDMode) 3;\
	$w.f3D.togl setconf -shade \$ay(cVDMode);\
	viewSetDModeIcon $w 3;\
	\$ay(currentView) mc"

if { $AYWITHAQUA } {
    $m entryconfigure 0 -image {} -label "Draw"
    $m entryconfigure 1 -image {} -label "Shade"
    $m entryconfigure 2 -image {} -label "Shade&Draw"
    $m entryconfigure 3 -image {} -label "HiddenWire"
}


# Grid Menu
if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
    menubutton $w.fMenu.g -image ay_Grid_img -menu $w.fMenu.g.m\
	    -padx 0 -pady 0 -borderwidth 0
    set gu $ayviewshortcuts(GridCycleUp)
    set gd $ayviewshortcuts(GridCycleDown)
    balloon_set $w.fMenu.g [string map [list <gu> <$gu> <gd> <$gd>] [ms vmenu4]]
    set m [menu $w.fMenu.g.m -tearoff 0]
    if { [winfo toplevel $w] == $w } {
	bind ayview <$aymainshortcuts(MenuMod)-g> "viewPostMenu %W.fMenu.g.m"
    }
    bind $w.fMenu.g <ButtonPress-1>\
	"+tk_menuSetFocus $w; catch \{destroy %W.balloon\}"
} else {
    set m [menu $mb.mgrid -tearoff 0]
    $mb add cascade -label Grid -menu $m
}
# if

$m add command -image ay_Grid0.1_img -hidemargin 1 -command "\
    $w.f3D.togl setconf -grid 0.1 -drawg 1 -ugrid 1;\
    viewSetGridIcon $w 0.1"
$m add command -image ay_Grid0.25_img -hidemargin 1 -command "\
    $w.f3D.togl setconf -grid 0.25 -drawg 1 -ugrid 1;\
    viewSetGridIcon $w 0.25"
$m add command -image ay_Grid0.5_img -hidemargin 1 -command "\
    $w.f3D.togl setconf -grid 0.5 -drawg 1 -ugrid 1;\
    viewSetGridIcon $w 0.5"
$m add command -image ay_Grid1.0_img -hidemargin 1 -command "\
    $w.f3D.togl setconf -grid 1.0 -drawg 1 -ugrid 1;\
    viewSetGridIcon $w 1.0"
$m add command -image ay_GridX_img -hidemargin 1 -command "\
    after idle \{$confm invoke 13\}"

$m add command -image ay_Grid_img -hidemargin 1 -command "\
    $w.f3D.togl setconf -grid 0.0 -drawg 0 -ugrid 0;\
    viewSetGridIcon $w 0.0"

if { $AYWITHAQUA } {
    $m entryconfigure 0 -image {} -label "Grid 0.1"
    $m entryconfigure 1 -image {} -label "Grid 0.25"
    $m entryconfigure 2 -image {} -label "Grid 0.5"
    $m entryconfigure 3 -image {} -label "Grid 1.0"
    $m entryconfigure 4 -image {} -label "Set Grid"
    $m entryconfigure 5 -image {} -label "No Grid"
}

if { 0 } {
set f $w.fMenu
if { $menubar } {
    set f $w.menubar
}
bind $f <ButtonPress-$aymainshortcuts(CMButton)>\
    "viewToggleModellingScope %W 1"
}

# Help menu (just for MacOSX/Aqua!)
if { $AYWITHAQUA && (! ([winfo toplevel $w] != $w)) } {
    set m [menu $mb.help -tearoff 0]
    $mb add cascade -label "Help" -menu $m
    $m add command -label "Help" -command "\$ay(helpmenu) invoke 0"
    $m add command -label "Help on object" \
	-command "\$ay(helpmenu) invoke 1"\
    $m add command -label "Show Shortcuts" -command "shortcut_show"
    $m add command -label "About" -command "aboutAyam"
    $m add checkbutton -label "Show Tooltips" -variable ayprefs(showtt)
}
# if

if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
    pack $w.fMenu.v $w.fMenu.t $w.fMenu.c -in $w.fMenu -side left

    pack $w.fMenu.g -in $w.fMenu -side right

    pack $w.fMenu.dm -in $w.fMenu -side right

    pack $w.fMenu.mm -in $w.fMenu -side right

    pack $w.fMenu.a -in $w.fMenu -side right

    # XXXX Windows Menus are a bit to tall
    global tcl_platform tcl_patchLevel
    if { $tcl_platform(platform) == "windows" } {
	set children [winfo children $w.fMenu]
	foreach child $children {
	    $child configure -pady 1
	}

	# fix image menus for Tk8.4
	if { $tcl_patchLevel > "8.3" } {
	    set m [$w.fMenu.a cget -menu]
	    $m entryconfigure 0 -hidemargin 0
	    set m [$w.fMenu.dm cget -menu]
	    $m entryconfigure 0 -hidemargin 0
	    set m [$w.fMenu.mm cget -menu]
	    $m entryconfigure 0 -hidemargin 0
	    set m [$w.fMenu.g cget -menu]
	    $m entryconfigure 0 -hidemargin 0
	}

    }
}
# if

return;
}
# vmenu_open


# vmenu_addbutton
#  add a button to the view menu
#
proc vmenu_addbutton { w b img cmd } {
global AYWITHAQUA

    set mb ""

    if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
	if { ! [winfo exists $w.fMenu.$b] } {
	    set mb [button $w.fMenu.$b -image $img -command $cmd \
			-padx 0 -pady 0 -borderwidth 0]
	    pack $mb -in $w.fMenu -side right
	}
    }

return $mb;
}
# vmenu_addbutton


proc _lbMove { lb1 lb2 } {
    set sel1 [$lb1 selection get]
    if { $sel1 != "" } {
	set text [$lb1 itemcget $sel1 -text]
	set img [$lb1 itemcget $sel1 -image]
	$lb1 delete $sel1
	set sel2 [$lb2 selection get]
	if { $sel2 == "" } {
	    set inspos end
	} else {
	    set inspos [$lb2 index $sel2]
	}
	$lb2 insert $inspos $sel1 -text $text -image $img
    }
 return;
}

proc _lbMoveAll { lb1 lb2 } {
    $lb1 selection clear
    foreach item [$lb1 items] {
	$lb2 insert end $item -text [$lb1 itemcget $item -text]\
	    -image [$lb1 itemcget $item -image]
	$lb1 delete $item
    }
 return;
}

proc _lbSel { lb item } {
    if { [$lb selection get] == $item } {
	$lb selection clear
    } else {
	$lb selection set $item
    }
 return;
}

proc _lbRes { lb items allitems allimgs } {
    $lb delete [$lb items]
    foreach item $items {
	set index [lsearch $allitems $item]
	set img [lindex $allimgs $index]
	$lb insert end $item -text "  $item" -image $img
    }
 return;
}


proc vmenu_configureactionmenu { } {
    global ay ayprefs

    set allitems [list "None" "Tag" "Pick" "SelBnd" "SelBndC" "TagB" "Mark" "FindU" "FindUV"]
    set allimgs [list "ay_EmptyG_img" "ay_Tag_img" "ay_Pick_img" "ay_SelBnd_img" "ay_SelBndC_img" "ay_TagB_img" "ay_Mark_img" "ay_FindU_img" "ay_FindUV_img"]

    winAutoFocusOff

    set w .mac
    set t "Set Action Menu"

    winDialog $w $t

    set gs $ay(GuiScale)

    set ay(bca) $w.f2.bca
    set ay(bok) $w.f2.bok

    # upper frame with two listboxes separated by a row of buttons
    set f [frame $w.f1]

    set fl [frame $f.fl]
    pack [label $fl.l -text "Available:"] -in $fl
    set px [expr (25 * round($gs)) + round(2 * $gs)]
    set lbl [ListBox $fl.lb -deltay [expr 30 + (30*(round($gs)-1))]\
		 -padx $px]

    set index 0
    set leftitems {}
    foreach item $allitems {
	if { [lsearch $ayprefs(VMenuList) $item] == -1 } {
	    set img [lindex $allimgs $index]
	    $lbl insert end $item -text "  $item" -image $img
	    lappend leftitems $item
	}
	incr index
    }

    $lbl bindText <1> "_lbSel $lbl"
    $lbl bindImage <1> "_lbSel $lbl"
    pack $lbl -in $fl
    pack $fl -in $f -side left -fill y

    # row of buttons between the listboxes
    set fm [frame $f.fm]
    set fn [list fixed [expr round(10*$gs)] bold]
    set btta [button $fm.ltrr -text " >> " -font $fn -width 3]
    set btt [button $fm.ltr -text " > " -font $fn -width 3]
    set btm [button $fm.res -text " O " -font $fn -width 3]
    set btl [button $fm.rtl -text " < " -font $fn -width 3]
    set btla [button $fm.rtll -text " << " -font $fn -width 3]
    pack $btta $btt $btm $btl $btla -in $fm
    pack $fm -in $f -side left

    set fr [frame $f.fr]
    pack [label $fr.l -text "Menu:"] -in $fr
    set lbr [ListBox $fr.lb -deltay [expr 30 + (30*(round($gs)-1))]\
		 -padx $px]
    foreach item $ayprefs(VMenuList) {
	set index [lsearch $allitems $item]
	set img [lindex $allimgs $index]
	$lbr insert end $item -text "  $item" -image $img
    }
    $lbr bindText <1> "_lbSel $lbr"
    $lbr bindImage <1> "_lbSel $lbr"

    pack $lbr -in $fr
    pack $fr -in $f -side left -fill y

    $btta conf -command "_lbMoveAll $lbl $lbr"
    $btt conf -command "_lbMove $lbl $lbr"
    $btl conf -command "_lbMove $lbr $lbl"
    $btla conf -command "_lbMoveAll $lbr $lbl"
    $btm conf -command "_lbRes {$lbl} {$leftitems} {$allitems} {$allimgs}; _lbRes {$lbr} {$ayprefs(VMenuList)} {$allitems} {$allimgs}"
    pack $f -in $w -side top -fill x

    set l [$lbl items]
    if { $l != "" } {
	set font [$lbl itemcget [lindex $l 0] -font]
    } else {
	set l [$lbr items]
	if { $l != "" } {
	    set font [$lbr itemcget [lindex $l 0] -font]
	}
    }
    if { $font != "" } {
	$lbl configure -width [expr ([font measure $font "A"]*10 + $px)/8]
	$lbr configure -width [expr ([font measure $font "A"]*10 + $px)/8]
    }

    # lower frame with "OK" "Cancel"
    set f [frame $w.f2]
    button $f.bok -text "Ok" -width 5 -command {
	global ayprefs
	set ayprefs(VMenuList) [.mac.f1.fr.lb items]
	foreach view $ay(views) {
	    vmenu_fillactionmenu $view
	}
	destroy .mac
    }
    # button

    button $f.bca -text "Cancel" -width 5 -command {
	destroy .mac
    }
    # button

    pack $f.bok $f.bca -in $f -side left -fill x -expand yes
    pack $f -in $w -side bottom -fill x

    # Esc-key && close via window decoration == Cancel button
    bind $w <Escape> "$f.bca invoke"
    wm protocol $w WM_DELETE_WINDOW "$f.bca invoke"

    winRestoreOrCenter $w $t
    focus $w.f2.bok

    # establish "Help"-binding
    shortcut_addcshelp $w ayam-2.html amconf

    tkwait window $w

    after idle viewMouseToCurrent

    winAutoFocusOn

 return;
}
# vmenu_configureactionmenu


proc vmenu_fillactionmenu { w } {
 global ay AYWITHAQUA ayprefs

    if { (! $AYWITHAQUA ) || ([winfo toplevel $w] != $w) } {
	set m $w.fMenu.a.m
    } else {
	set m $w.menubar.ma
    }

    $m delete 0 end
    foreach item $ayprefs(VMenuList) {
	switch $item {
	    "Pick" {
		$m add command -image ay_Pick_img -hidemargin 1\
		    -command "actionPick $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "Pick"
		}
	    }
	    "Tag" {
		$m add command -image ay_Tag_img -hidemargin 1\
		    -command "actionTagP $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "Tag"
		}
	    }
	    "None" {
		$m add command -image ay_EmptyG_img -hidemargin 1\
		    -command "actionClear $w.f3D.togl 1"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "None"
		}
	    }
	    "Mark" {
		$m add command -image ay_Mark_img -hidemargin 1\
		    -command "actionSetMark $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "Mark"
		}
	    }
	    "SelBnd" {
		$m add command -image ay_SelBnd_img -hidemargin 1\
		    -command "actionSelBnd $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "SelBnd"
		}
	    }
	    "SelBndC" {
		$m add command -image ay_SelBndC_img -hidemargin 1\
		    -command "actionSelBnd $w.f3D.togl 1"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "SelBndC"
		}
	    }
	    "TagB" {
		$m add command -image ay_TagB_img -hidemargin 1\
		    -command "actionTagB $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "TagB"
		}
	    }
	    "FindU" {
		$m add command -image ay_FindU_img -hidemargin 1\
		    -command "actionFindU $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "FindU"
		}
	    }
	    "FindUV" {
		$m add command -image ay_FindUV_img -hidemargin 1\
		    -command "actionFindUV $w.f3D.togl"
		if { $AYWITHAQUA } {
		    $m entryconfigure end -image {} -label "FindUV"
		}
	    }
	}
	# switch
    }
    # foreach
    if { $ay(ws) == "Windows" } {
	$m add command -image blindmenu -state disabled
    }
 return;
}
# vmenu_fillactionmenu
