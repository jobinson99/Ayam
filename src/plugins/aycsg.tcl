# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2004 by Randolf Schultz
# (rschultz@informatik.uni-rostock.de) and others.
#
# All rights reserved.
#
# See the file License for details.

# aycsg.tcl - aycsg CSG preview plugin Tcl code

global ay ayprefs

# the aycsg icon
set imgdata {\
R0lGODlhGQAZAOcAAGZiVqKahuLWttbKrsbCsoJ6Zsa2nrqulpKKdtra2q6ijvLiwraqkm5qWpqa
mu7ewo6GcpKSknZyYqKint7Ssr6ymrKmkpqSfurevoqCbqamnqqiisK+soJ+es7Cpsq+oop+bure
wtrKrnZybqKWgo6OiubautbGqoaGhqqehsK2mm5uZnpyYp6WftLGqn56cn52Zraulq6qqpaOeq6m
jm5mVtrOss6+poZ+aoqGgqaekqaehuLStsa+qoaCfuraut7OsmpmVnJuYp6SfqqmppKGdrauoubW
upaWksq6ou7evqaahtLCqsa6nu7iwn5+dqamonZ2bsKymp6emmpmXrKqkqqqptrOupaKdq6iinp2
bsq+pnJuZpqOerKmjnZyanJqWraqlqKiooJ6bsa6opaSkubavo6GdvLixt7StmpmWmpiVnZuXoJ+
duLWuoqCcoJ6aqqeisK2npaOfuLSuqamptbGrnpyZp6Wgm5mWnJqXuravoZ+bp6enoJ+fpaKenp2
csa+rpaWlnJuav//////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////ywAAAAAGQAZAAAI/gATCBxI
sKDBgnUSKkw4wU+DEQ2eoBCzUKFAMRgzIhkEoAEMPjDY5Fnzok9GjAIrttGToUiXC3gCDPlzBo6Q
EgtTJnTwAsYfPDsUWLAQo0ocmRDAPEkoRqeYPHC6pGBQQU6TD0maqKjgZcidNTkSXqwThcWcHQyk
bPHgYoAdF1t6tKiRR0sBlAnqRFBTJIWFA1uYiABCgYINERCgIuiCR0cdgVAGwcDzlwwTGzyOaBYQ
RwKEISkUMIjRNIEYvnEOkLkxAIgZDD/2nGBRhASNMAcqGCAiMEKeGV6keBDBw8SeB0oedMlAWQWZ
Hh88QBHoQMIFL3Iu8/ixwMkCGwWG/mQ58MHFiQE2HieYwGKIAhWXj+xR4j0Ogh2qXQzOrL4PGDxZ
VPDBADxg8MACaJzhXgXDuWHcEbyZlgcCqX1gR3F7YDBAGyQwkERrR2DghA2lTaBGBgHEYIALNrhh
xh4qZEBCGBamkeECBKgnhhYwdFGFCjecAIQAR0gBwRLkDUCBfCFYUZpeNRQRRxgqbKEfEAqgCJgd
QOwRAhli5VXHC1rMgJZaLrgwBA54MNAEl0e4wZROE+QBwwxZhFEBGVuccUcXFbpwBZhhYlRHH2vA
gEAAoh2wRB5nkKAAgz3UQdGTllq6AhggzBBTHIlegJYRmYbJlKEOfAEGDHBkUIAaHSDMQRFTluqk
kBhQHOqDBCMAgoAgl1Z00LDEChQQADs=
}
image create photo ay_aycsg_img -format GIF -data $imgdata

image create photo ay_aycsg_img2 -format GIF -data $imgdata
set color #e02020
ay_aycsg_img2 put $color -to 0 0 24 3
ay_aycsg_img2 put $color -to 0 0 3 24
ay_aycsg_img2 put $color -to 22 24 24 0
ay_aycsg_img2 put $color -to 0 22 24 24


if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_aycsg_img
    _zoomImage ay_aycsg_img2
}


# aycsgToggle:
#  toggle use of AyCSG as regular drawing mode
#  also manage corresponding view menu button
proc aycsgToggle { w } {
    set b ${w}.fMenu.baycsg
    if { [$b cget -image] == "ay_aycsg_img2" } {
	$b configure -image ay_aycsg_img
    } else {
	$b configure -image ay_aycsg_img2
    }
    if { [string first ".view" $w] == 0 } {
	set togl [winfo toplevel $w].f3D.togl
    } else {
	set togl ${w}.f3D.togl
    }
    $togl togglecsg
 return;
}
# aycsgToggle

# add icon to view menus
set wh [expr round($ay(GuiScale)) * 22]
set a "\
button \$w.fMenu.baycsg -width $wh -height $wh -padx 0 -pady 0 -borderwidth 0 -image ay_aycsg_img -command \{global ay; \$ay(currentView) rendercsg;\};pack \$w.fMenu.baycsg -in \$w.fMenu -side right;"
append a "bind \$w.fMenu.baycsg <Shift-1> \"aycsgToggle \$w\""

addToProc vmenu_open $a

foreach view $ay(views) {
    set w $view
    eval $a
}

# add key binding to view windows to initiate the CSG drawing
set a "bind \$w <Control-C> {global ay; \$ay(currentView) rendercsg;}"
addToProc shortcut_viewactions $a

foreach view $ay(views) {
    bind ${view} <Control-C>\
	    {global ay; $ay(currentView) rendercsg;}
}
# foreach


# code for the aycsg preferences dialog

uplevel #0 {
    array set aycsg_options {
	Algorithm 1
	DCSampling 1
	OffscreenType 0
	Optimization 0
	CalcBBS 0
    }
    # aycsg_options

    array set aycsg_options_save [array get aycsg_options]

}
# uplevel

# aycsgPreferences:
#  AyCSG preferences dialog
proc aycsgPreferences { } {
    global ay aycsg_options aycsg_options_save

    winAutoFocusOff

    set aycsg_options(oldfocus) [focus]

    array set aycsg_options_save ""
    array set aycsg_options_save [array get aycsg_options]

    set w .aycsgprefs
    set t "AyCSG Preferences"
    winDialog $w $t

    set f [frame $w.f1]
    pack $f -in $w -side top -fill x

    set ay(bca) .aycsgprefs.f2.bca
    set ay(bok) .aycsgprefs.f2.bok

    addMenu $f aycsg_options_save Algorithm [list Automatic Goldfeather SCS]
    addMenu $f aycsg_options_save DCSampling \
	[list NoDCSampling OcclusionQuery DCSampling]
    addMenu $f aycsg_options_save OffscreenType \
	[list Automatic FBO PBuffer FBOARB FBOEXT]
    addMenu $f aycsg_options_save Optimization \
	[list Default ForceOn On Off]
    addCheck $f aycsg_options_save CalcBBS

    set f [frame $w.f2]
    button $f.bok -text "Ok" -width 5 -command {
	global ay ayprefs aycsg_options aycsg_options_save

	if { $aycsg_options_save(DCSampling) == 2 } {
	    # check, whether views will be created with stencil buffers,
	    # if not: ask the user for permission to add the needed parameters
	    if { ($ayprefs(AddViewParams) == "") ||
	         ([string first stencil $ayprefs(AddViewParams)] == -1) } {
		set t "Adapt Preferences?"
		set m "Ayam is not set up to use stencil buffers,\n\
			but this is needed by the depth complexity\n\
			sampling strategy you have chosen (DCSampling)!\n\
			Do you want me to adapt the preferences?"
		set answer [tk_messageBox -title $t -type yesno \
			-icon question -message $m]
		if { $answer == "no" } {
		    set aycsg_options_save(DCSampling) 0
		} else {
		    set ayprefs(AddViewParams) "-stencil true -stencilsize 8"
		    update
		    tk_messageBox -title "Information" -type ok\
		    -icon info -message "Stencil buffers are now enabled\n\
		    for every new view window!"
		}
		# if
	    }
	    # if
	}
	# if

	array set aycsg_options [array get aycsg_options_save]
	grab release .aycsgprefs
	restoreFocus $aycsg_options(oldfocus)
	destroy .aycsgprefs
    }
    # button
    button $f.bca -text "Cancel" -width 5 -command "\
		grab release .aycsgprefs;\
		restoreFocus $aycsg_options(oldfocus);\
		destroy .aycsgprefs"

    pack $f.bok $f.bca -in $f -side left -fill x -expand yes
    pack $f -in $w -side bottom -fill x

    # Esc-key && close via window decoration == Cancel button
    bind $w <Escape> "$f.bca invoke"
    wm protocol $w WM_DELETE_WINDOW "$f.bca invoke"

    # establish "Help"-binding
    shortcut_addcshelp $w ayam-8.html aycsg

    bind $w <Key-Return> "$::ay(bok) invoke;break"
    catch {bind $w <Key-KP_Enter> "$::ay(bok) invoke;break"}

    winRestoreOrCenter $w $t
    grab $w
    focus $w.f2.bok
    tkwait window $w

    winAutoFocusOn

    after idle viewMouseToCurrent

    unset aycsg_options_save

    aycsgSetOpt

    # XXXX loop through all views and rerender
    # when continuous CSG drawing is enabled

 return;
}
# aycsgPreferences


# add aycsg-preferences dialog to custom menu
set m $ay(cm)
$m add command -label "AyCSG Preferences" -command aycsgPreferences

# we always need an open view (OpenGL context) upon startup for the
# GLEW initialization, if there is none, we open it here
if { $ay(views) == "" } {
    set t "Open View?"
    set m\
"Initialization of AyCSG requires an open view window!\nMay I open one now?"
    set answer [tk_messageBox -title $t -type okcancel -icon question\
	    -message $m]
    if { $answer == "ok" } {
	viewOpen 400 300
	update
    }
}
# if

# realize preference settings
aycsgSetOpt
