# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2021 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# link.tcl - manually control linking of view(s) and main window

# set the keyboard shortcut to toggle linking:
set link_hotkey "<F7>"

catch {rename ::plb_update ::link_plb_update}
catch {rename ::rV ::link_rV}
set ay(couple) 1

proc plb_update { } {
    global ay
    if { $ay(couple) || ![link_focus] } {
	::link_plb_update
    }
 return;
}


proc rV { args } {
    global ay
    if { $ay(couple) || [link_focus] } {
	::link_rV $args
    }
 return;
}


proc link_focus { } {
    global ayprefs ay

    if { $ayprefs(SingleWindow) } {
	# single window
	if { [string first ".view" [focus]] == 0 } {
	    return 1;
	} else {
	    if { [string first ".fv" [focus]] == 0 } {
		return 1;
	    } else {
		if { [string first ".fu.fMain.fview3" [focus]] == 0 } {
		    return 1;
		}
	    }
	    return 0;
	}
    } else {
	# multi window
	if { [string first ".view" [focus]] == 0 } {
	    return 1;
	}
    }

 return 0;
}

proc link_toggle { } {
    global ay
    if { $ay(couple) } {
	uS 1 1; rV;
    }
    return;
}

proc link_togglekbd { } {
    global ay
    set ay(couple) [expr {!$ay(couple)}]
    link_toggle
    return;
}

image create photo ay_Link_img -format GIF -data {\
R0lGODlhDQANAIABAAAAAE5OTiH5BAEKAAEALAAAAAANAA0AAAIajB8AyKfd
HlOUQrduynPJrm1hVUWc9qHnUQAAOw==}

if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_Link_img
}

checkbutton .fu.fMain.lcb -variable ay(couple) -command link_toggle\
    -image ay_Link_img\
    -ind false -selectcolor "" -rel flat\
    -overrel raised -offrel flat -borderwidth 1

balloon_set .fu.fMain.lcb "Link Hierarchy with Views"

addToProc shortcut_fkeys {
    bind $w <[repctrl $aymainshortcuts(Update)]>\
     {
	 ayError 4 $aymainshortcuts(Update) "Update!"
	 set ay(coupleorig) $ay(couple)
	 set ay(couple) 1
	 notifyOb -all; uS 1 1; rV "" 1;
	 set ay(couple) $ay(coupleorig)
     }
}

catch { bind . $link_hotkey link_togglekbd }
catch { shortcut_addviewbinding $link_hotkey link_togglekbd }

_placeTreeIcon .fu.fMain.lcb

return;
# EOF
