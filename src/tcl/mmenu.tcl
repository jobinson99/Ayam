# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2005 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# mmenu.tcl - the main menu

# mmenu_open:
#  create the Ayam main application menu
#
proc mmenu_open { w } {

global ay ayprefs AYWITHAQUA

if { ! $AYWITHAQUA } {
    frame $w.fMenu -bd 2 -relief raised
    pack $w.fMenu -side top -fill both -expand no
    # File
    menubutton $w.fMenu.fil -text "File" -menu $w.fMenu.fil.m -padx 3\
	-underline 0

    menu $w.fMenu.fil.m -tearoff 0
    set m $w.fMenu.fil.m
} else {
    set mb [menu $w.menubar -tearoff 0 -type menubar]
    . configure -menu $mb

    # correct application menu (about entry)
    menu $w.menubar.apple
    $w.menubar add cascade -menu $w.menubar.apple
    $w.menubar.apple add command -label "About Ayam" -command "aboutAyam"

    # File
    set m [menu $mb.mfil -tearoff 0]
    $mb add cascade -label "File" -menu $m -underline 0
}
# if

set ay(filemenu) $m

$m add command -label "New" -command {
    if { ! [io_warnChanged] } {
	cS; plb_update; goTop
	set ay(CurrentLevel) "root"
	set ay(SelectedLevel) "root"
	newScene; uS;
	if { ($ayprefs(EnvFile) != "") && $ayprefs(NewLoadsEnv) == 1 } {
	    viewCloseAll; cS; plb_update
	    set filename [file nativename $ayprefs(EnvFile)]
	    if { $tcl_platform(platform) == "windows" } {
		regsub -all {\\} $filename {/} filename
	    }
	    catch {replaceScene $filename}
	    update
	}
	set ay(filename) ""
	wm title . "Ayam - Main - : --"
	update
	uS
	update
	set ay(sc) 0
	update
	foreach view $ay(views) { viewBind $view }
	update
	rV
	after idle viewMouseToCurrent
    }
} -underline 0
$m add separator
$m add command -label "Open" -command {
    if { ! [io_warnChanged] } {
	io_replaceScene
    }
} -underline 0
$m add command -label "Insert" -command "io_insertScene" -underline 0
$m add separator
$m add command -label "Save as" -command "io_saveScene ask 0" -underline 1
$m add command -label "Save" -command "io_saveScene dontask 0" -underline 0
$m add separator
$m add cascade -menu $m.im -label "Import" -underline 1
menu $m.im -tearoff 0
set ay(im) $m.im
$m add separator
$m add cascade -menu $m.ex -label "Export" -underline 0
menu $m.ex -tearoff 0
set ay(em) $m.ex
$m.ex add command -label "RenderMan RIB" -command "io_exportRIB" -underline 0
$m add separator
$m add command -label "Load Plugin" -command "io_loadCustom" -underline 0
$m add separator
$m add command -label "Save Prefs" -command "prefs_save" -underline 5

$m add separator

set label "1. [lindex $ayprefs(mru) 0]"
$m add command -label $label -command {
    if { ! [io_warnChanged] } { io_mruLoad 0 }
} -underline 0
set label "2. [lindex $ayprefs(mru) 1]"
$m add command -label $label -command {
    if { ! [io_warnChanged] } { io_mruLoad 1 }
} -underline 0
set label "3. [lindex $ayprefs(mru) 2]"
$m add command -label $label -command {
    if { ! [io_warnChanged] } { io_mruLoad 2 }
} -underline 0
set label "4. [lindex $ayprefs(mru) 3]"
$m add command -label $label -command {
    if { ! [io_warnChanged] } { io_mruLoad 3 }
} -underline 0
$m add separator
$m add command -label "Exit!" -command io_exit -underline 1

#
# Edit Menu
#
if { ! $AYWITHAQUA } {
    pack $w.fMenu.fil -in $w.fMenu -side left
    menubutton $w.fMenu.ed -text "Edit" -menu $w.fMenu.ed.m -padx 3\
	-underline 0
    menu $w.fMenu.ed.m -tearoff 0
    set m $w.fMenu.ed.m
} else {
    set m [menu $mb.medi -tearoff 0]
    $mb add cascade -label "Edit" -menu $m -underline 0
}
set ay(editmenu) $m
$m add command -label "Copy" -command { copOb } -underline 0
$m add command -label "Cut" -command {
    cutOb; cS; notifyOb -parent;
    set ay(ul) $ay(CurrentLevel); uS; rV; set ay(sc) 1
} -underline 2
$m add command -label "Paste" -command {
    pasOb; uCR; sL $ay(ucrcount); notifyOb -parent; rV; set ay(sc) 1
} -underline 0
$m add command -label "Delete" -command {
    if { [candelOb] == 1 } {
	delOb; cS; notifyOb -parent;
	set ay(ul) $ay(CurrentLevel); uS; rV; set ay(sc) 1
    } else {
	ayError 2 "Delete" "Can not delete these objects."
    }
} -underline 0
$m add separator
$m add command -label "Select All" -command {
    if { $ay(lb) == 0 } {
	set tree $ay(tree)
	set nodes [$tree nodes $ay(CurrentLevel)]
	if { $ay(CurrentLevel) == "root" } {
	    set nodes [lrange $nodes 1 end]
	}
	if { [llength $nodes] > 0 } {
	    eval [subst "$tree selection set $nodes"]
	    eval [subst "treeSelect $nodes"]
	    # allow dnd of multiple selected objects
	    $tree bindText <ButtonPress-1> ""
	    $tree bindText <ButtonRelease-1> "tree_selectItem $tree"
	}
	plb_update
	if { $ay(need_redraw) } {
	    rV
	}
    } else {
	$ay(olbball) invoke
	break
    }
} -underline 8
#  ^^^^^^^^^^^ l
$m add command -label "Select None" -command {
    cS; plb_update
    if { $ay(need_redraw) == 1 } {
	rV
    }
} -underline 7
#  ^^^^^^^^^^^ n
$m add separator
$m add command -label "Copy Property" -command { pclip_copy 0 }
$m add command -label "Copy Marked Prop" -command { pclip_copy 1 }
$m add command -label "Paste Property" -command {
    pclip_paste; set ay(sc) 1
}
$m add separator
$m add command -label "Undo" -command {
    set oldfocus [focus]
    undo; uCL cl "0 1"; plb_update; rV;
    after idle "focus $oldfocus"
} -underline 0
$m add command -label "Redo" -command {
    set oldfocus [focus]
    undo redo; uCL cl "0 1"; plb_update; rV;
    after idle "focus $oldfocus"
} -underline 0
$m add separator
$m add command -label "Material" -command { material_edit } -underline 0
$m add command -label "Master" -command { instance_edit } -underline 1
$m add separator
$m add command -label "Search" -command { objectsearch_open } -underline 1
$m add separator
$m add command -label "Preferences" -command { prefs_open; rV } -underline 3

if { ! $AYWITHAQUA } {
    pack $w.fMenu.ed -in $w.fMenu -side left
    # Create
    menubutton $w.fMenu.cr -text "Create" -menu $w.fMenu.cr.m -padx 3\
	-underline 0
    menu $w.fMenu.cr.m -tearoff 0
    set m $w.fMenu.cr.m
} else {
    # Create
    set m [menu $mb.mcrt -tearoff 0]
    $mb add cascade -label "Create" -menu $m -underline 0
}

set ay(createmenu) $m

$m add cascade -menu $m.cu -label "Curve" -underline 0
menu $m.cu -tearoff 0
$m.cu add command -label "NURBCurve" -command {
    runTool {ay(nclen) ay(ncadda)} {"Length:" "AddArgs:"}\
	"crtOb NCurve -length %0 %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NCurve" {ayam-6.html sccrtnc}
} -underline 4
#  ^^^^^^^^^^^ => C
$m.cu add command -label "ICurve" -command {
    runTool {ay(iclen) ay(ncadda)} {"Length:" "AddArgs:"}\
	"crtOb ICurve -length %0 %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create ICurve" {ayam-6.html sccrtic}
} -underline 0
$m.cu add command -label "ACurve" -command {
    runTool {ay(aclen) ay(ncadda)} {"Length:" "AddArgs:"}\
	"crtOb ACurve -length %0 %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create ACurve" {ayam-6.html sccrtac}
} -underline 0
$m.cu add command -label "NCircle" -command {
    runTool {ay(ncircradius) ay(ncirctmin) ay(ncirctmax)}\
	{"Radius:" "TMin:" "TMax:"}\
 "crtOb NCircle -radius %0 -tmin %1 -tmax %2; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NCircle"
} -underline 3
#  ^^^^^^^^^^^ => R

$m.cu add separator

$m.cu add command -label "Circular B-Spline" -command {
    runTool { ay(cbsprad) ay(cbsptmax) ay(cbspsec) ay(cbsporder) }\
	{"Radius:" "Arc:" "Sections:" "Order:"}\
    "crtClosedBS -r %0 -a %1 -s %2 -o %3; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create Closed B-Spline"
} -underline 9
$m.cu add command -label "Rectangle" -command {
    runTool {ay(nrectwidth) ay(nrectheight)} {"Width:" "Height:"}\
	"crtNRect -w %0 -h %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create Rectangle"
} -underline 1
$m.cu add command -label "TrimRect" -command {
    crtTrimRect; set ay(ul) $ay(CurrentLevel); uS 0 1; rV
} -underline 0

$m.cu add separator

$m.cu add command -label "ConcatNC" -command "level_crt ConcatNC;" \
    -underline 2
$m.cu add command -label "OffsetNC" -command "level_crt OffsetNC \"\" -1;" \
    -underline 0
$m.cu add command -label "ExtrNC" -command "level_crt ExtrNC \"\" -1;" \
    -underline 1

$m add cascade -menu $m.su -label "Surface" -underline 0
menu $m.su -tearoff 0
$m.su add command -label "NURBPatch" -command {
    runTool {ay(npwidth) ay(npheight) ay(npadda)}\
	{"Width:" "Height:" "AddArgs:"}\
   "crtOb NPatch -width %0 -height %1 %2; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NPatch" {ayam-6.html sccrtnp}
} -underline 0
$m.su add command -label "IPatch" -command {
    runTool {ay(ipwidth) ay(ipheight) ay(ipadda)}\
	{"Width:" "Height:" "AddArgs:"}\
   "crtOb IPatch -width %0 -height %1 %2; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create IPatch" {ayam-6.html sccrtip}
} -underline 0
$m.su add command -label "APatch" -command {
    runTool {ay(apwidth) ay(apheight) ay(apadda)}\
	{"Width:" "Height:" "AddArgs:"}\
   "crtOb APatch -width %0 -height %1 %2; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create APatch" {ayam-6.html sccrtap}
} -underline 0
$m.su add command -label "BPatch" -command {
    crtOb BPatch; uCR; sL; placeOb; notifyOb; uP; rV;
} -underline 0
$m.su add command -label "PatchMesh" -command {
    runTool {ay(pmwidth) ay(pmheight) ay(npadda)}\
	{"Width:" "Height:" "AddArgs:"}\
"crtOb PatchMesh -width %0 -height %1 %2; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create PatchMesh" {ayam-6.html sccrtpm}
} -underline 0

$m.su add separator

$m.su add command -label "NURBSphere" -command {
    runTool ay(nsphereradius) "Radius:"\
	"crtNSphere -r %0; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NURBSphere"
} -underline 4

$m.su add command -label "Revolve" -command "level_crt Revolve;" -underline 0
$m.su add command -label "Extrude" -command "level_crt Extrude;" -underline 0
$m.su add command -label "Sweep" -command "level_crt Sweep; sweep_rotcross"\
    -underline 1
$m.su add command -label "Swing" -command "level_crt Swing; swing_rotcross;"\
    -underline 4
$m.su add command -label "Cap" -command "level_crt Cap;" -underline 1
$m.su add command -label "Bevel" -command "level_crt Bevel;" -underline 2
$m.su add command -label "Birail1" -command "level_crt Birail1;" -underline 6
$m.su add command -label "Birail2" -command "level_crt Birail2;" -underline 6
$m.su add command -label "Gordon" -command "level_crt Gordon;" -underline 3
$m.su add command -label "Skin" -command "level_crt Skin;" -underline 1
$m.su add command -label "Trim" -command "level_crt Trim;" -underline 0

$m.su add separator

$m.su add command -label "ConcatNP" -command "level_crt ConcatNP;" \
    -underline 0
$m.su add command -label "OffsetNP" -command "level_crt OffsetNP \"\" -1;" \
     -underline 0
$m.su add command -label "ExtrNP" -command "level_crt ExtrNP \"\" -1;" \
    -underline 1

$m add cascade -menu $m.sl -label "Solid" -underline 4
menu $m.sl -tearoff 0
$m.sl add command -label "Box"\
    -command "crtOb Box; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Sphere"\
    -command "crtOb Sphere; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Disk"\
    -command "crtOb Disk; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Cone"\
    -command "crtOb Cone; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Cylinder"\
    -command "crtOb Cylinder; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 1
$m.sl add command -label "Torus"\
    -command "crtOb Torus; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Hyperboloid"\
    -command "crtOb Hyperboloid; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.sl add command -label "Paraboloid"\
    -command "crtOb Paraboloid; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0

$m add cascade -menu $m.csg -label "Level" -underline 0
menu $m.csg -tearoff 0
$m.csg add command -label "Level"\
    -command "crtOb Level 1; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.csg add separator
$m.csg add command -label "Union"\
    -command "crtOb Level 2; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.csg add command -label "Intersection"\
    -command "crtOb Level 4; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.csg add command -label "Difference"\
    -command "crtOb Level 3; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 0
$m.csg add separator
$m.csg add command -label "Primitive"\
    -command "crtOb Level 5; uCR; sL; notifyOb; rV;" -underline 0

$m add command -label "Light"\
    -command "crtOb Light; uCR; sL; placeOb; notifyOb; uP; rV;"\
    -underline 1
$m add separator
$m add cascade -menu $m.cus -label "Custom Object" -underline 1
menu $m.cus -tearoff 0

$m add separator
$m add command -label "View" \
    -command "viewOpen 400 300; global ay; set ay(ul) root:0; uS 0 1; rV;\
		after idle viewMouseToCurrent" \
    -underline 0
$m add separator
$m add command -label "Instance" \
    -command "crtInstances; uCR; sL; placeOb; notifyOb; uP; rV;" \
    -underline 1
#    ^^^^^^^^^^^ => N
$m add command -label "Clone" -command "level_crt Clone;" \
    -underline 2
#    ^^^^^^^^^^^ => O
$m add command -label "Mirror" -command "level_crt Mirror;" \
    -underline 2
#    ^^^^^^^^^^^ => R
$m add command -label "Material" \
	-command "material_createp;" -underline 0
$m add command -label "Camera" \
	-command "crtOb Camera; uCR; sL; placeOb; notifyOb; uP; rV;" -underline 1
$m add command -label "RiInc" \
	-command "crtOb RiInc; uCR; sL; placeOb; notifyOb; uP; rV;"
$m add command -label "RiProc" \
	-command "crtOb RiProc; uCR; sL; placeOb; notifyOb; uP; rV;"
$m add command -label "Script" \
	-command "crtOb Script; uCR; sL; notifyOb; rV;" \
    -underline 5
#    ^^^^^^^^^^^ => T
$m add command -label "Select" \
    -command "crtOb Select; uCR; sL; notifyOb; rV;" \
    -underline 1
#    ^^^^^^^^^^^ => E
$m add command -label "Text" \
    -command "crtOb Text; uCR; sL; placeOb; notifyOb; uP; rV;" \
    -underline 2
#    ^^^^^^^^^^^ => X

if { ! $AYWITHAQUA } {
    pack $w.fMenu.cr -in $w.fMenu -side left
    # Tools
    menubutton $w.fMenu.tool -text "Tools" -menu $w.fMenu.tool.m -padx 3\
	-underline 0
    set m [menu $w.fMenu.tool.m -tearoff 0]
} else {
    # Tools
    set m [menu $mb.mtools -tearoff 0]
    $mb add cascade -label "Tools" -menu $m -underline 0
}

set ay(toolsmenu) $m
$m add command -label "Last (None)"

$m add separator

$m add cascade -menu $m.nct -label "Curve" -underline 0
menu $m.nct -tearoff 0

$m.nct add command -label "Revert" -command { undo save RevertC; revertC;
                                              plb_update; rV } \
 -underline 0
# ^^^^^^^^^^^ => R

$m.nct add command -label "Open" -command { undo save Open; openC;
                                            plb_update; rV } \
 -underline 0
# ^^^^^^^^^^^ => O

$m.nct add command -label "Close" -command { undo save Close; closeC;
                                            plb_update; rV } \
 -underline 0
# ^^^^^^^^^^^ => C

$m.nct add command -label "Make Periodic" -command { undo save "Make Periodic";
                                            ncurve_makeperiodic;
                                            plb_update; rV } \
 -underline 5
# ^^^^^^^^^^^ => P

$m.nct add command -label "Concat" -command {
    runTool {ay(concatc) ay(concatt) ay(concatf) ay(concatflen)}\
    {"Close:" "Knot-Type:" "Fillets:" "FilletLength:"}\
     "concatC -c %0 -k %1 -f %2 -fl %3; uCR; rV" "Concatenate Curves" concatt
} -underline 2
# ^^^^^^^^^^^ => N
$m.nct add command -label "Split" -command {
    runTool {ay(splitu) ay(splitr) ay(splitappend)}\
	{"Split at:" "Relative:" "Append:"}\
	"npatch_split splitNC %0 %1 %2;" "Split Curve" splitt
} -underline 0
#  ^^^^^^^^^^^ => S

$m.nct add command -label "Trim" -command {
    runTool [list ay(trimumin) ay(trimumax)]\
	[list "UMin:" "UMax:"]\
	"undo save TrimNC; trimNC %0 %1; plb_update; rV"\
	"Trim Curve" ctrimt
} -underline 0
#  ^^^^^^^^^^^ => T

$m.nct add command -label "Refine" -command { undo save Refine;
    refineC; plb_update; rV } \
 -underline 2
# ^^^^^^^^^^^ => F

$m.nct add command -label "Coarsen" -command { undo save Coarsen; coarsenC;
                                              plb_update; rV } \
 -underline 2
# ^^^^^^^^^^^ => A

$m.nct add command -label "Clamp" -command { undo save ClampNC; clampNC;
                                             plb_update; rV } \
 -underline 3
# ^^^^^^^^^^^ => M

$m.nct add command -label "Unclamp" -command { undo save UnclampNC; unclampNC;
                                             plb_update; rV } \
 -underline 0
# ^^^^^^^^^^^ => U

$m.nct add command -label "Elevate" -command {
    runTool ay(elevd) {"Elevate by:"}\
	"undo save Elevate; elevateNC %0; plb_update; rV"\
	"Elevate Curve" elevt
} -underline 1
#  ^^^^^^^^^^^ => L

$m.nct add command -label "Reduce" -command {
    runTool ay(reduct) {"Tolerance:"}\
	"undo save Reduce; reduceNC %0; plb_update; rV"\
	"Reduce Curve" reducet
} -underline 2
#  ^^^^^^^^^^^ => E

$m.nct add command -label "Extend" -command {
    runTool [list ay(extendx) ay(extendy) ay(extendz)] [list "X:" "Y:" "Z:"]\
	"undo save Extend; extendNC %0 %1 %2; plb_update; rV"\
	"Extend Curve" extendt
} -underline 1
#  ^^^^^^^^^^^ => X

$m.nct add command -label "Interpolate" -command {
    runTool [list ay(interpo) ay(interpc) ay(interppt) ay(interpsdl) ay(interpedl)]\
	[list "Order:" "Closed:" "ParamType:" "SDLen:" "EDLen:"]\
      "undo save InterpNC; interpNC -o %0 -c %1 -k %2 -s %3 -e %4; plb_update; rV"\
	"Interpolate Curve" interpnct
}

$m.nct add command -label "Approximate" -command {
    runTool [list ay(appro) ay(apprl) ay(apprc) ay(apprt)]\
	[list "Order:" "Length:" "Closed:" "TessParam:"]\
      "undo save ApproxNC; approxNC -o %0 -l %1 -c %2 -t %3; plb_update; rV"\
	"Approximate Curve" approxnct
}

$m.nct add command -label "Fair" -command {
    runTool [list ay(fairtol) ay(fairworst)]\
	[list "Tolerance:" "Fair Worst:"]\
	"undo save FairNC; if { $ay(fairworst) } {fairNC -w %0;} else {fairNC %0;}; plb_update; rV"\
	"Fair Curve" fairnct
}

$m.nct add cascade -menu $m.nct.kn -label "Knots" -underline 0
menu $m.nct.kn -tearoff 0

$m.nct.kn add command -label "Insert Knot" -command {
    getType ay(typ) 1; if { $ay(typ) == "NCurve" } {
	getProp; set ay(okn) $::NCurveAttrData(Knots);
    }
    runTool [list ay(okn) ay(insknu) ay(insknr)]\
	[list "Old knots:" "Insert knot at:" "Insert times:"]\
	"undo save InsKn; insknNC %1 %2; plb_update; rV"\
	"Insert Knot" insknt
} -underline 0

$m.nct.kn add command -label "Remove Knot" -command {
    getType ay(typ) 1; if { $ay(typ) == "NCurve" } {
	getProp; set ay(okn) $::NCurveAttrData(Knots);
    }
    if { [info exists ::u] } {
	set ay(remknu_l) $::u
    }
    runTool [list ay(okn) ay(remknu) ay(remknr) ay(remtol)]\
	[list "Old knots:" "Remove knot:" "Remove times:" "Tolerance:"]\
	"undo save RemKn; remknNC %1 %2 %3; plb_update; rV"\
	"Remove Knot" remknt
} -underline 0

$m.nct.kn add command -label "Remove Superfluous Knots" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save RemSuKn; remsuknNC %0; plb_update; rV"\
	"Remove Superfluous Knots" remsuknt
} -underline 2

$m.nct.kn add command -label "Refine Knots" -command { undo save RefineKn;
    refineknNC; plb_update; rV } -underline 1

$m.nct.kn add command -label "Refine Knots with" -command {
    getType ay(typ) 1; if { $ay(typ) == "NCurve" } {
	getProp; set ay(okn) $::NCurveAttrData(Knots); ncurve_getrknots;
    }
    runTool [list ay(okn) ay(refinekn)] {"Old knots:" "New knots:"}\
	"undo save RefineKn; refineknNC \{%1\}; plb_update; rV"\
        "Refine Knots" refknit
} -underline 2

$m.nct.kn add command -label "Rescale Knots to Range" -command {
    undo save RescaleKnots;
    runTool {ay(rmin) ay(rmax)} {"RangeMin:" "RangeMax:"}\
	"rescaleknNC -r %0 %1; plb_update;"\
	"Rescale Knots" resckrt
} -underline 2

$m.nct.kn add command -label "Rescale Knots to Mindist" -command {
    undo save RescaleKnots;
    runTool ay(mindist) "MinDist:"\
	"rescaleknNC -d %0; plb_update;"\
	"Rescale Knots" resckmt
} -underline 3

$m.nct.kn add command -label "Reparameterize" -command {
    undo save Reparameterize;
    runTool ay(reparamtype) "Knot-Type:"\
	"reparamNC %0; plb_update;"\
	"Reparameterize" reparamt
} -underline 3

$m.nct add command -label "Reset Weights" -command {
    if { $ay(views) != "" } {
	undo save ResetWeights
	[lindex $ay(views) 0].f3D.togl wrpac
	rV
    } else {
	ayError 2 "Reset_Weights" "Need an open view!"
    }
}

$m.nct add command -label "Plot Curvature" -command {
    runTool [list ay(curvatp) ay(curvatw) ay(curvats)]\
	[list "Data points:" "Width:" "Scale Height:"]\
	"curvPlot %0 %1 %2; uCR; rV"\
	"Curvature Plot" plotcurt
}
$m.nct add command -label "Shift" -command {
    runTool ay(shiftcbsp) {"Times:"}\
	"undo save ShiftC; shiftC %0; rV"\
	"Shift Curve" shiftclc
} -underline 2
#  ^^^^^^^^^^^ => I

$m.nct add command -label "Tween" -command {
    runTool { ay(tweenr) ay(tweenappend) } {"Ratio:" "Append:"}\
	"undo save TweenNC; npatch_split tweenNC %0 0 %1 npatch_tweensel"\
	"Tween Curves" tweennct
} -underline 2
#  ^^^^^^^^^^^ => E

$m.nct add command -label "To XY" -command {
    undo save ToXYNC; toXYNC; rV; } \
 -underline 4
# ^^^^^^^^^^^ => Y

$m.nct add command -label "Make Compatible" -command {
    runTool ay(clevel) {"Level:"}\
	"undo save MakeCompNC; makeCompNC -l %0; rV"\
	"Make Compatible" makecompt
}

$m add separator

$m add cascade -menu $m.npt -label "Surface" -underline 0
menu $m.npt -tearoff 0
$m.npt add command -label "Revert U" -command {
    undo save RevertU; revertuS; plb_update; rV
}
$m.npt add command -label "Revert V" -command {
    undo save RevertV; revertvS; plb_update; rV
}
$m.npt add command -label "Swap UV" -command {
    undo save SwapUV; swapuvS; plb_update; rV
}

$m.npt add command -label "Close U" -command {
    undo save CloseUNP; closeuNP; plb_update; rV
}
$m.npt add command -label "Close V" -command {
    undo save CloseVNP; closevNP; plb_update; rV
}

$m.npt add command -label "Split U" -command {
    runTool {ay(splitu) ay(splitr) ay(splitappend)}\
	{"Split at:" "Relative:" "Append:"}\
	"undo save SplitUNP; npatch_split splituNP %0 %1 %2"\
	"Split Surface U" splitnpt
}

$m.npt add command -label "Split V" -command {
    runTool {ay(splitu) ay(splitr) ay(splitappend)}\
	{"Split at:" "Relative:" "Append:"}\
	"undo save SplitVNP; npatch_split splitvNP %0 %1 %2"\
	"Split Surface V" splitnpt
}


$m.npt add cascade -menu $m.npt.cl -label "Clamp/Unclamp" -underline 0
menu $m.npt.cl -tearoff 0

$m.npt.cl add command -label "Clamp U" -command {
    undo save ClampUNP; clampuNP; plb_update; rV
}
$m.npt.cl add command -label "Clamp V" -command {
    undo save ClampVNP; clampvNP; plb_update; rV
}
$m.npt.cl add command -label "Clamp Both" -command {
    undo save ClampNP; clampuNP; clampvNP; plb_update; rV
}

$m.npt.cl add command -label "Unclamp U" -command {
    undo save UnclampUNP; unclampuNP; plb_update; rV
}
$m.npt.cl add command -label "Unclamp V" -command {
    undo save UnclampVNP; unclampvNP; plb_update; rV
}
$m.npt.cl add command -label "Unclamp Both" -command {
    undo save UnclampNP; unclampuNP; unclampvNP; plb_update; rV
}


$m.npt add cascade -menu $m.npt.el -label "Elevate/Reduce" -underline 0
menu $m.npt.el -tearoff 0

$m.npt.el add command -label "Elevate U" -command {
    runTool ay(elevnpu) "Elevate U by:"\
	"undo save ElevateUNP; elevateuNP %0; plb_update; rV"\
	"Elevate Surface U" elevuvt
}

$m.npt.el add command -label "Elevate V" -command {
    runTool ay(elevnpv) "Elevate V by:"\
	"undo save ElevateVNP; elevatevNP %0; plb_update; rV"\
	"Elevate Surface V" elevuvt
}

$m.npt.el add command -label "Elevate Both" -command {
    runTool [list ay(elevnpu) ay(elevnpv)]\
	[list "Elevate U by:" "Elevate V by:"]\
	"undo save ElevateUVNP; elevateuNP %0; elevatevNP %1; plb_update; rV"\
	"Elevate Surface" elevuvt
}

$m.npt.el add command -label "Reduce U" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save ReduceUNP; reduceuNP %0; plb_update; rV"\
	"Reduce Surface U" reduceuvt
}

$m.npt.el add command -label "Reduce V" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save ReduceVNP; reducevNP %0; plb_update; rV"\
	"Reduce Surface V" reduceuvt
}

$m.npt.el add command -label "Reduce Both" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save ReduceUVNP; reduceuNP %0; reducevNP %1; plb_update; rV"\
	"Reduce Surface" reduceuvt
}


$m.npt add cascade -menu $m.npt.in -label "Interpolate" -underline 0
menu $m.npt.in -tearoff 0

$m.npt.in add command -label "Interpolate U" -command {
    runTool ay(interpou) {"Order:"}\
	"undo save InterpUNP; interpuNP -o %0; plb_update; rV"\
	"Interpolate Surface U" interpnpt
}

$m.npt.in add command -label "Interpolate V" -command {
    runTool ay(interpov) {"Order:"}\
	"undo save InterpVNP; interpvNP -o %0; plb_update; rV"\
	"Interpolate Surface V" interpnpt
}

$m.npt.in add command -label "Interpolate Both" -command {
    runTool [list ay(interpou) ay(interpov)]\
	[list "Order U:" "Order V:"]\
      "undo save InterpUVNP; interpuNP -o %0; interpvNP -o %1; plb_update; rV"\
	"Interpolate Surface" interpnpt
}


$m.npt add cascade -menu $m.npt.ap -label "Approximate" -underline 0
menu $m.npt.ap -tearoff 0

$m.npt.ap add command -label "Approximate U" -command {
    runTool [list ay(apprl) ay(appro) ay(apprc) ay(apprk) ay(apprt)]\
	[list "Width:" "Order_U:" "Closed_U:" "KnotType_U:" "TessParam:"]\
    "undo save ApproxNP; approxNP -aw %0 -ou %1 -cu %2 -ktu %3 -tp %4; plb_update; rV"\
	"Approximate Surface U" approxnpt
}

$m.npt.ap add command -label "Approximate V" -command {
    runTool [list ay(apprl) ay(appro) ay(apprc) ay(apprk) ay(apprt)]\
	[list "Height:" "Order_V:" "Closed_V:" "KnotType_V:" "TessParam:"]\
 "undo save ApproxNP; approxNP -m 1 -ah %0 -ov %1 -cv %2 -ktv %3 -tp %4; plb_update; rV"\
	"Approximate Surface V" approxnpt
}

$m.npt.ap add command -label "Approximate UV" -command {
    runTool [list ay(apprw) ay(apprh) ay(appruo) ay(apprvo) ay(appruc) ay(apprvc) ay(appruk) ay(apprvk) ay(apprut) ay(apprvt)]\
	[list "Width:" "Height:" "Order_U:" "Order_V:" "Closed_U:" "Closed_V:" "KnotType_U:" "KnotType_V:" "TessParam_U:" "TessParam_V:"]\
    "undo save ApproxNP; approxNP -m 2 -aw %0 -ah %1 -ou %2 -ov %3 -cu %4 -cv %5 -ktu %6 -ktv %7 -tpu %8 -tpv %9; plb_update; rV"\
	"Approximate Surface UV" approxnpt
}

$m.npt.ap add command -label "Approximate VU" -command {
    runTool [list ay(apprw) ay(apprh) ay(appruo) ay(apprvo) ay(appruc) ay(apprvc) ay(appruk) ay(apprvk) ay(apprut) ay(apprvt)]\
	[list "Width:" "Height:" "Order_U:" "Order_V:" "Closed_U:" "Closed_V:" "KnotType_U:" "KnotType_V:" "TessParam_U:" "TessParam_V:"]\
    "undo save ApproxNP; approxNP -m 3 -aw %0 -ah %1 -ou %2 -ov %3 -cu %4 -cv %5 -ktu %6 -ktv %7 -tpu %8 -tpv %9; plb_update; rV"\
	"Approximate Surface VU" approxnpt
}


$m.npt add cascade -menu $m.npt.kn -label "Knots" -underline 0
menu $m.npt.kn -tearoff 0

$m.npt.kn add command -label "Insert Knot U" -command {
    runTool [list ay(insknu) ay(insknr)]\
	[list "Insert knot at:" "Insert times:"]\
	"undo save InsKnUNP; insknuNP %0 %1; plb_update; rV"\
	"Insert Knot U" insknnpt
}

$m.npt.kn add command -label "Insert Knot V" -command {
    runTool [list ay(insknu) ay(insknr)]\
	[list "Insert knot at:" "Insert times:"]\
	"undo save InsKnVNP; insknvNP %0 %1; plb_update; rV"\
	"Insert Knot V" insknnpt
}

$m.npt.kn add command -label "Remove Knot U" -command {
    getType ay(typ) 1; if { $ay(typ) == "NPatch" } {
	getProp; set ay(okn) $::NPatchAttrData(Knots_U);
    }
    runTool [list ay(okn) ay(remknu) ay(remknr) ay(remtol)]\
	[list "Old knots:" "Remove knot:" "Remove times:" "Tolerance:"]\
	"undo save RemKnU; remknuNP %1 %2 %3; plb_update; rV"\
	"Remove Knot U" remknnpt
}

$m.npt.kn add command -label "Remove Knot V" -command {
    getType ay(typ) 1; if { $ay(typ) == "NPatch" } {
	getProp; set ay(okn) $::NPatchAttrData(Knots_V);
    }
    runTool [list ay(okn) ay(remknu) ay(remknr) ay(remtol)]\
	[list "Old knots:" "Remove knot:" "Remove times:" "Tolerance:"]\
	"undo save RemKnV; remknvNP %1 %2 %3; plb_update; rV"\
	"Remove Knot V" remknnpt
}

$m.npt.kn add command -label "Remove Superfluous Knots U" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save RemSuKnU; remsuknuNP %0; plb_update; rV"\
	"Remove Superfluous Knots U" remsuknuvt
}

$m.npt.kn add command -label "Remove Superfluous Knots V" -command {
    runTool ay(remstol) "Tolerance:"\
	"undo save RemSuKnV; remsuknvNP %0; plb_update; rV"\
	"Remove Superfluous Knots V" remsuknuvt
}

$m.npt.kn add command -label "Refine Knots U" -command {
    undo save RefineUNP; refineuNP; plb_update; rV
}

$m.npt.kn add command -label "Refine Knots U with" -command {
    getType ay(typ) 1; if { $ay(typ) == "NPatch" } {
	getProp; set ay(okn) $::NPatchAttrData(Knots_U); npatch_getrknotsu;
    }
    runTool [list ay(okn) ay(refineknu)] {"Old knots:" "New knots:"}\
	"undo save RefineKnUNP; refineuNP \{%1\}; plb_update; rV"\
        "Refine Knots U" refinest
}

$m.npt.kn add command -label "Refine Knots V" -command {
    undo save RefineKnVNP; refinevNP; plb_update; rV
}

$m.npt.kn add command -label "Refine Knots V with" -command {
    getType ay(typ) 1; if { $ay(typ) == "NPatch" } {
	getProp; set ay(okn) $::NPatchAttrData(Knots_V); npatch_getrknotsv;
    }
    runTool [list ay(okn) ay(refineknv)] {"Old knots:" "New knots:"}\
	"undo save RefineKnVNP; refinevNP \{%1\}; plb_update; rV"\
        "Refine Knots V" refinest
}

$m.npt.kn add command -label "Rescale Knots to Range U" -command {
    undo save RescaleKnots 1;
    runTool {ay(rmin) ay(rmax)} {"RangeMin:" "RangeMax:"}\
	"rescaleknNP -ru %0 %1; plb_update;"\
	"Rescale Knots U" resckrnpt
}
$m.npt.kn add command -label "Rescale Knots to Range V" -command {
    undo save RescaleKnots 1;
    runTool {ay(rmin) ay(rmax)} {"RangeMin:" "RangeMax:"}\
	"rescaleknNP -rv %0 %1; plb_update;"\
	"Rescale Knots V" resckrnpt
}
$m.npt.kn add command -label "Rescale Knots to Range Both" -command {
    undo save RescaleKnots 1;
    runTool {ay(rmin) ay(rmax)} {"RangeMin:" "RangeMax:"}\
	"rescaleknNP -r %0 %1; plb_update;"\
	"Rescale Knots" resckrnpt
}
$m.npt.kn add command -label "Rescale Knots to Mindist U" -command {
    undo save RescaleKnots 1;
    runTool ay(mindist) "MinDist:"\
	"rescaleknNP -du %0; plb_update;"\
	"Rescale Knots U" resckmnpt
}
$m.npt.kn add command -label "Rescale Knots to Mindist V" -command {
    undo save RescaleKnots 1;
    runTool ay(mindist) "MinDist:"\
	"rescaleknNP -dv %0; plb_update;"\
	"Rescale Knots V" resckmnpt
}
$m.npt.kn add command -label "Rescale Knots to Mindist Both" -command {
    undo save RescaleKnots 1;
    runTool ay(mindist) "MinDist:"\
	"rescaleknNP -d %0; plb_update;"\
	"Rescale Knots" resckmnpt
}

$m.npt add command -label "Fair" -command {
    runTool [list ay(fairmod) ay(fairtol) ay(fairworst)]\
	[list "Mode:" "Tolerance:" "Fair Worst:"]\
	"undo save FairNP; fairNP -m %0 -t %1 -w %2; plb_update; rV"\
	"Fair Surface" fairnpt
}

$m.npt add command -label "Reset Weights" -underline 0 -command {
    if { $ay(views) != "" } {
	undo save ResetWeights
	[lindex $ay(views) 0].f3D.togl wrpac
	rV
    } else {
	ayError 2 "Reset_Weights" "Need an open view!"
    }
}
$m.npt add command -label "Make Compatible" -underline 0 -command {
    runTool {ay(side) ay(clevel)} {"Side:" "Level:"}\
	"undo save MakeCompNP; makeCompNP -s %0 -l %1; rV"\
	"Make Compatible" makecompst
}
$m.npt add command -label "Tween" -underline 1 -command {
    runTool { ay(tweenr) ay(tweenappend) } {"Ratio:" "Append:"}\
	"undo save TweenNP; npatch_split tweenNP %0 0 %1 npatch_tweensel"\
	"Tween Surfaces" tweennpt
}
$m.npt add command -label "Tesselate" -command tgui_open -underline 0
$m.npt add separator
$m.npt add command -label "Break into Curves" -command npatch_break
$m.npt add command -label "Build from Curves" -command npatch_build

$m add separator

$m add cascade -menu $m.pm -label "PolyMesh" -underline 4
menu $m.pm -tearoff 0
$m.pm add command -label "Merge" -command { pomesh_merge } -underline 0
$m.pm add command -label "Split" -command { pomesh_split } -underline 0
$m.pm add command -label "Optimize" -command { pomesh_optimize } -underline 0
$m.pm add command -label "Connect" -command {
    selPnts -count ay(pmoffpnts)
    set ay(pmoff1label) "Offset1 ("
    append ay(pmoff1label) [lindex $ay(pmoffpnts) 0] " points):"
    set ay(pmoff2label) "Offset2 ("
    append ay(pmoff2label) [lindex $ay(pmoffpnts) 1] " points):"
    runTool {ay(pmoff1) ay(pmoff2)} [list $ay(pmoff1label) $ay(pmoff2label)]\
	"undo save ConnectPo; connectPo -o1 %0 -o2 %1; uCR; sL; notifyOb; rV;"\
	"Connect Edges" {ayam-2.html polymeshtools}
} -underline 0
$m.pm add command -label "Quadrangulate" -command { undo save QuadPo; quadPo; rV } -underline 0
$m.pm add separator
$m.pm add command -label "Gen. Face Normals" -command {
    undo save GenFaceNorm; genfnPo; rV; plb_update
} -underline 5
$m.pm add command -label "Gen. Smooth Normals" -command {
    undo save GenSmoothNorm; gensnPo; rV; plb_update
} -underline 0
$m.pm add command -label "Rem. Smooth Normals" -command {
    undo save RemSmoothNorm; remsnPo; rV; plb_update
} -underline 0
$m.pm add command -label "Flip Smooth Normals" -command {
    undo save FlipSmoothNorm; flipPo 1; rV
} -underline 0
$m.pm add separator
$m.pm add command -label "Flip Loops" -command {
    undo save FlipLoops; flipPo 2; rV
} -underline 1


$m add separator

$m add cascade -menu $m.pnt -label "Points" -underline 0
set sm [menu $m.pnt -tearoff 0]
$sm add command -label "Select All Points" -command "selPnts -all; rV" \
    -underline 0
$sm add command -label "Deselect All Points" -command "selPnts; rV" \
    -underline 0
$sm add command -label "Invert Selection" -command "invPnts; rV" \
    -underline 0

$sm add separator

$sm add command -label "Collapse Points" -command {collMP; rV; set ay(sc) 1}\
    -underline 0
$sm add command -label "Explode Points" -command {explMP; rV; set ay(sc) 1}\
    -underline 0

$sm add separator

$sm add command -label "Apply Trafo To All Points"\
    -command "undo save ApplyTrafo; applyTrafo; plb_update; rV" \
    -underline 0
$sm add command -label "Apply Trafo To Selected Points"\
    -command "undo save ApplyTrafo; applyTrafo -sel; plb_update; rV" \
    -underline 1
$sm add command -label "Center All Points (3D)"\
    -command "undo save CenterPnts; centerPnts; plb_update; rV" \
    -underline 1
$sm add command -label "Center All Points (2D-XY)"\
	-command "undo save CenterPntsXY; centerPnts 1;	plb_update; rV" \
    -underline 22
#    ^^^^^^^^^^^^ => X
$sm add command -label "Center All Points (2D-ZY)"\
	-command "undo save CenterPntsZY; centerPnts 2;	plb_update; rV" \
    -underline 23
#    ^^^^^^^^^^^^ => Y
$sm add command -label "Center All Points (2D-XZ)"\
	-command "undo save CenterPntsXZ; centerPnts 3;	plb_update; rV" \
    -underline 23
#    ^^^^^^^^^^^^ => Z

$m add separator

$m add cascade -menu $m.nc -label "Create" -underline 1
menu $m.nc -tearoff 0
$m.nc add command -label "Circular B-Spline" -command {
    runTool { ay(cbsprad) ay(cbsptmax) ay(cbspsec) ay(cbsporder) }\
	{"Radius:" "Arc:" "Sections:" "Order:"}\
      "crtClosedBS -r %0 -a %1 -s %2 -o %3; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create Closed B-Spline" cbspt
} -underline 9
$m.nc add command -label "NURBCircle" -command {
    runTool {ay(ncircradius) ay(ncircarc)} {"Radius:" "Arc:"}\
	"crtNCircle -r %0 -a %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NURBCircle" ncirct
} -underline 4
$m.nc add command -label "Rectangle" -command {
    runTool {ay(nrectwidth) ay(nrectheight)} {"Width:" "Height:"}\
	"crtNRect -w %0 -h %1; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create Rectangle" nrrt
} -underline 1
$m.nc add command -label "TrimRect" -command {
    crtTrimRect; set ay(ul) $ay(CurrentLevel); uS 0 1; rV
} -underline 0

$m.nc add separator

$m.nc add command -label "ConcatNC" -command "level_crt ConcatNC;" \
    -underline 2
$m.nc add command -label "OffsetNC" -command "level_crt OffsetNC \"\" -1;" \
    -underline 0
$m.nc add command -label "ExtrNC" -command "level_crt ExtrNC \"\" -1;" \
    -underline 1

$m.nc add separator

$m.nc add command -label "ConcatNP" -command "level_crt ConcatNP;"
$m.nc add command -label "OffsetNP" -command "level_crt OffsetNP \"\" -1;" \
     -underline 1

$m.nc add command -label "ExtrNP" -command "level_crt ExtrNP \"\" -1;"

$m.nc add command -label "NURBSphere" -command {
    runTool ay(nsphereradius) "Radius:"\
	"crtNSphere -r %0; uCR; sL; placeOb; notifyOb; uP; rV;"\
	"Create NURBSphere"
} -underline 4
$m.nc add command -label "NURBSphere2" -command {
    crtNSphere2; uCR; sL; placeOb; notifyOb; uP; rV
}
$m.nc add separator
$m.nc add command -label "Revolve" -command "level_crt Revolve;" -underline 0
$m.nc add command -label "Extrude" -command "level_crt Extrude;" -underline 0
$m.nc add command -label "Sweep" -command\
    "level_crt Sweep; sweep_rotcross" -underline 1
$m.nc add command -label "Swing" -command\
    "level_crt Swing; swing_rotcross;" -underline 4
$m.nc add command -label "Cap" -command "level_crt Cap;" -underline 1
$m.nc add command -label "Bevel" -command "level_crt Bevel;" -underline 2
$m.nc add command -label "Birail1" -command "level_crt Birail1;" -underline 6
$m.nc add command -label "Birail2" -command "level_crt Birail2;" -underline 6
$m.nc add command -label "Gordon" -command "level_crt Gordon;" -underline 0
$m.nc add command -label "Skin" -command "level_crt Skin;" -underline 1
$m.nc add command -label "Trim" -command "level_crt Trim;" -underline 3

$m add separator

$m add command -label "Hide/Show" -command {
    undo save HideShow
    hideOb -t
    set ay(ul) $ay(CurrentLevel); uS 1 1; rV
} -underline 0
$m add command -label "Hide All" -command "hideOb -all; uS 1 1; rV" \
    -underline 1
$m add command -label "Show All" -command "showOb -all; uS 1 1; rV" \
    -underline 3
$m add separator
$m add command -label "Convert" -command {
    convOb; update; cS; notifyOb;
    set ay(ul) $ay(CurrentLevel); uS; rV; set ay(sc) 1
} -underline 3
$m add command -label "Convert (In Place)" -command {
    set ay(need_undo_clear) 0
    forAll -recursive 0 { if { [hasChild] } { set ::ay(need_undo_clear) 1 } }
    if { $ay(need_undo_clear) == 0 } { undo save Convert }
    convOb -inplace; notifyOb; update
    if { $ay(need_undo_clear) } { undo clear }
    set ay(ul) $ay(CurrentLevel); uS 1 1; rV
}
$m add separator
$m add command -label "Force Notification" -command "notifyOb; rV" \
    -underline 0

if { ! $AYWITHAQUA } {
    pack $w.fMenu.tool -in $w.fMenu -side left

    # Custom
    menubutton $w.fMenu.cust -text "Custom" -menu $w.fMenu.cust.m -padx 3\
	-underline 1
    menu $w.fMenu.cust.m -tearoff 0
    pack $w.fMenu.cust -in $w.fMenu -side left
    set ay(cm) $w.fMenu.cust.m
    # Special
    menubutton $w.fMenu.spec -text "Special" -menu $w.fMenu.spec.m -padx 3\
	-underline 0
    set m [menu $w.fMenu.spec.m -tearoff 0]
    pack $w.fMenu.spec -in $w.fMenu -side left
} else {
    # Custom
    set m [menu $mb.mcust -tearoff 0]
    $mb add cascade -label "Custom" -menu $m -underline 1
    set ay(cm) $m
    # Special
    set m [menu $mb.mspecial -tearoff 0]
    $mb add cascade -label "Special" -menu $m -underline 0
}
set ay(specialmenu) $m

$m add command -label "Save Selected as" -command "io_saveScene ask 1" \
    -underline 0
$m add command -label "Save Environment" -command "io_saveEnv" \
    -underline 5

$m add cascade -menu $m.clp -label "Clipboard" -underline 0
set sm [menu $m.clp -tearoff 0]
$sm add command -label "Paste (Move)" -command {
    pasOb -move; cS; set ay(ul) $ay(CurrentLevel); uS; rV; set ay(sc) 1
} -underline 0
$sm add command -label "Replace" -command {
    repOb; cS; set ay(ul) $ay(CurrentLevel); uS; rV; set ay(sc) 1
} -underline 0
$sm add command -label "Copy (Add)" -command {copOb -add} -underline 1
$sm add command -label "Cut (Add)" -command {cutOb -add} -underline 1
$sm add command -label "Clear" -command "clearClip" -underline 1
$sm add separator
$sm add command -label "Paste Property to Selected" \
    -command "pclip_pastetosel; notifyOb; rV" -underline 18

$m add cascade -menu $m.ins -label "Instances" -underline 0
set sm [menu $m.ins -tearoff 0]
$sm add command -label "Resolve all Instances" -command "ai_open 1" \
    -underline 0
$sm add command -label "Automatic Instancing" -command "ai_open 0" \
    -underline 0

$m add cascade -menu $m.tag -label "Tags" -underline 0
set sm [menu $m.tag -tearoff 0]
$sm add command -label "Add RiOption" -command "riopt_addp" -underline 6
# o
$sm add command -label "Add RiAttribute" -command "riattr_addp" -underline 6
# a
$sm add command -label "Edit TexCoords" -command "tc_edit" -underline 5
# t
$sm add command -label "Add Property" -command "plb_addremprop" -underline 4
# p

$sm add command -label "Remove Property" -command "plb_addremprop 1" \
    -underline 0

# r
$sm add command -label "Add Tag" -command "addTagp" \
    -underline 6
# g

#$m add command -label "Create ShadowMaps" -command "riopt_addp"

$m add cascade -menu $m.rib -label "RIB-Export" -underline 5
set sm [menu $m.rib -tearoff 0]
$sm add command -label "From Camera" -command "io_exportRIBfC" \
    -underline 5
#    ^^^^^^^^^^^ => C
$sm add command -label "Selected Objects" -command "io_exportRIBSO" \
    -underline 9
#    ^^^^^^^^^^^ => O
$sm add command -label "Create All ShadowMaps" -command "io_RenderSM . 1" \
    -underline 7
#    ^^^^^^^^^^^ => A
$sm add command -label "Create ShadowMap" -command "io_RenderSM . 0" \
    -underline 7
#    ^^^^^^^^^^^ => S
$m add separator
$m add command -label "Enable Scripts" -command "script_enable" \
    -underline 2
#    ^^^^^^^^^^^ => A
$m add command -label "Select Renderer" -command "render_select" \
    -underline 7
#    ^^^^^^^^^^^ => R
$m add command -label "Scan Shaders" -command "shader_scanAll" \
    -underline 3
#    ^^^^^^^^^^^ => N
$m add command -label "Reset Preferences" -command "prefs_reset" \
    -underline 6
#    ^^^^^^^^^^^ => P
$m add command -label "Reset Layout" -command "\
if \{\$ay(ws)==\"X11\" \} \{toolbox_layout\};winRestorePaneLayout \"\"" \
    -underline 6
#    ^^^^^^^^^^^ => L
$m add separator
$m add command -label "Toggle Toolbox" -command "toolbox_toggle" \
    -underline 11
#    ^^^^^^^^^^^^ => B
$m add command -label "Toggle TreeView" -command "tree_toggle" \
    -underline 11
#    ^^^^^^^^^^^^ => V

$m add separator

$m add command -label "Zap Ayam" -command "zap" -underline 0

if { ! $AYWITHAQUA } {
    # Help
    menubutton $w.fMenu.hlp -text "Help" -menu $w.fMenu.hlp.m -padx 3 \
	-underline 0
    set m [menu $w.fMenu.hlp.m -tearoff 0]
    pack $w.fMenu.hlp -in $w.fMenu -side right
} else {
    # Help
    set m [menu $mb.help -tearoff 0]
    $mb add cascade -label "Help" -menu $m -underline 0
}
set ay(helpmenu) $m

$m add command -label "Help" -command {
    after idle {
	global ayprefs
	openUrl $ayprefs(Docs)
    }
} -underline 0

$m add command -label "Help on object" -command {
    after idle {
	catch {
	    global ayprefs
	    set selected ""
	    getSel selected
	    if { $selected == "" } {
		ayError 2 "Help on object" "Please select an object!"
		return;
	    }
	    getType type
	    set type [string tolower $type]
	    openUrl [concatUrls ${ayprefs(Docs)} ayam-4.html\#${type}obj]
	}
    }
} -underline 8
#  ^^^^^^^^^^^ => O

$m add command -label "Help on property" -command {
    after idle {
	catch {
	    global ay ayprefs
	    set selected ""
	    getSel selected
	    if { $selected == "" } {
		ayError 2 "Help on property" "Please select an object!"
		return;
	    }

	    set lb $ay(plb)
	    set index [$lb curselection]

	    if { $index == "" } {
		ayError 2 "Help on property" "No property selected!"
		return;
	    }

	    set type [$lb get $index]
	    set type [string tolower $type]
	    openUrl [concatUrls ${ayprefs(Docs)} ayam-4.html\#${type}prop]
	}
    }
} -underline 8
#  ^^^^^^^^^^^ => P

$m add command -label "Show Shortcuts" -command "shortcut_show" \
    -underline 0
$m add command -label "About" -command "aboutAyam" \
    -underline 0
$m add checkbutton -label "Show Tooltips" -variable ayprefs(showtt) \
    -underline 5
#    ^^^^^^^^^^^ => T

# XXXX Win32 Menus are a bit to tall
global tcl_platform
if { $tcl_platform(platform) == "windows" } {
    set children [winfo children $w.fMenu]
    foreach child $children {
	$child configure -pady 1
    }
}

mmenu_addlume $ay(toolsmenu).nc
mmenu_addlume $ay(toolsmenu).nct
mmenu_addlume $ay(toolsmenu).nct.kn
mmenu_addlume $ay(toolsmenu).npt
mmenu_addlume $ay(toolsmenu).npt.cl
mmenu_addlume $ay(toolsmenu).npt.el
mmenu_addlume $ay(toolsmenu).npt.in
mmenu_addlume $ay(toolsmenu).npt.ap
mmenu_addlume $ay(toolsmenu).npt.kn
mmenu_addlume $ay(toolsmenu).pm
mmenu_addlume $ay(toolsmenu).pnt
mmenu_addlume $ay(toolsmenu)

# add traces that change the undo menu entries according to the operations
# that can be undone or redone
trace variable ay(undoo) w mmenu_updateundo
trace variable ay(redoo) w mmenu_updateundo

return;
}
# mmenu_open


# mmenu_updateundo:
#  update undo main menu entries
proc mmenu_updateundo {n1 n2 op} {
    global ay
    $ay(editmenu) entryconfigure 12 -label "Undo ($ay(undoo))"
    $ay(editmenu) entryconfigure 13 -label "Redo ($ay(redoo))"
}
# mmenu_updateundo


# mmenu_addcustom:
#  XXXX add code to avoid creating double entries when
#  custom objects are overloaded?
proc mmenu_addcustom { name command } {
    global ay
    set m $ay(createmenu).cus
#    set m $w.fMenu.cr.m.cus
    $m add command -label $name -command $command

 return;
}
# mmenu_addcustom


# mmenu_addlume:
#  arrange for menu <m> to update the "last used tool" menu entry
#  when a command entry in this menu is used
proc mmenu_addlume { m } {
    global ay
    set i 0
    # avoid processing ourselves
    if { $m == $ay(toolsmenu) } {
	incr i
    }
    set last [$m index end]
    while { $i <= $last } {
	if { [$m type $i] == "command" } {
	    set newlabel "Last ("
	    append newlabel [$m entrycget $i -label]
	    append newlabel ")"
	    set cmd "$ay(toolsmenu) entryconfigure 0 -label \"$newlabel\" \
                     -command \{[$m entrycget $i -command]\};"
	    append cmd "set ay(repo) \{[$m entrycget $i -label]\};"
	    append cmd [$m entrycget $i -command]
	    $m entryconfigure $i -command $cmd
	}
	# if
	incr i
    }
    # while
 return;
}
# mmenu_addlume
