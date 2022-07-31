# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2015 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# aysdr.tcl - aysdr shader plugin GUI code

# aysdr_rewritepath:
# helper for aysdr_scansdrtcmd
# changes '...;C:/bla...' to '...;//C/bla...'
proc aysdr_rewritepath { {path ""} } {
    global ay ayprefs

    if { $path == "" } {
	set t [split $ayprefs(Shaders) \;]
    } else {
	set t [split $path \;]
    }
    set p ""

    foreach elem $t {
	set ind [string first ":" $elem]
	if { $ind > 0 } {
	    append p "//"
	    append p [string index $elem 0]
	    append p [string range $elem 2 end]
	} else {
	    append p $elem
	}
	append p ";"
    }

    set ay(PixieShaders) $p

 return;
}
# aysdr_rewritepath

proc aysdr_manageShaderEnv { {mode "set"} } {
    global ay ayprefs env RiOptData tcl_platform
    if { $mode == "set" } {
	if { $tcl_platform(platform) == "windows" } {
	    aysdr_rewritepath [shader_unglobShaderPaths $ayprefs(Shaders)]
	} else {
	    set ay(PixieShaders) [shader_unglobShaderPaths $ayprefs(Shaders)]
	}
    }
    set oldSel [getSel]
    cl root
    selOb 0
    getProp
    if { $mode == "set" } {
	set ay(SavedShaders) $RiOptData(Shaders)
	if { $RiOptData(Shaders) != "" } {
	    append RiOptData(Shaders) ":" $ay(PixieShaders)
	} else {
	    set RiOptData(Shaders) $ay(PixieShaders)
	}
    } else {
	# unset
	set RiOptData(Shaders) $ay(SavedShaders)
    }
    setProp
    cl -
    if { $oldSel != "" } {
	selOb $oldSel
    } else {
	selOb
    }
 return;
}

set ::ayprefs(QRenderPre) {aysdr_manageShaderEnv}
set ::ayprefs(QRenderPost) {aysdr_manageShaderEnv unset}
set ::ayprefs(RenderPre) {aysdr_manageShaderEnv}
set ::ayprefs(RenderPost) {aysdr_manageShaderEnv unset}
set ::ayprefs(FRenderPre) {aysdr_manageShaderEnv}
set ::ayprefs(FRenderPost) {aysdr_manageShaderEnv unset}

