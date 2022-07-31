# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2021 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# apatch.tcl - approximating curves objects Tcl code

# apatch_getAttr:
#  get Attributes from C context and build new PropertyGUI
#
proc apatch_getAttr { } {
    global ay ayprefs APatchAttr APatchAttrData

    set oldfocus [focus]

    catch {destroy $ay(pca).$APatchAttr(w)}
    set w [frame $ay(pca).$APatchAttr(w)]

    getProp

    set ay(bok) $ay(appb)

    # create new UI
    set a $APatchAttr(arr)

    addVSpace $w s1 2
    addParam $w $a Width
    addParam $w $a Height

    addVSpace $w s2 4
    addMenu $w $a Mode [list UV VU U V]
    addParam $w $a AWidth
    addParam $w $a AHeight
    addParam $w $a Order_U
    addParam $w $a Order_V
    if { $APatchAttrData(Mode) == 3 } {
	addMenu $w $a Knot-Type_U [list Chordal Centripetal\
				       Bezier B-Spline NURB]
    } else {
	addMenu $w $a Knot-Type_U [list Chordal Centripetal]
    }
    if { $APatchAttrData(Mode) == 2 } {
	addMenu $w $a Knot-Type_V [list Chordal Centripetal\
				       Bezier B-Spline NURB]
    } else {
	addMenu $w $a Knot-Type_V [list Chordal Centripetal]
    }
    if { $APatchAttrData(Mode) != 3 } {
	addCheck $w $a Close_U
    }
    if { $APatchAttrData(Mode) != 2 } {
	addCheck $w $a Close_V
    }
    addVSpace $w s3 4
    addParam $w $a Tolerance
    addMenu $w $a DisplayMode $ay(npdisplaymodes)

    addVSpace $w s4 4
    addText $w $a "Created NURBS Patch:"
    addInfo $w $a NPInfo

    plb_setwin $w $oldfocus

 return;
}
# apatch_getAttr


proc init_APatch { } {
    global APatch_props APatchAttr APatchAttrData

    set APatch_props { Transformations Attributes Material Tags Bevels\
			   Caps APatchAttr }

    array set APatchAttr {
	arr   APatchAttrData
	sproc ""
	gproc apatch_getAttr
	w     fAPatchAttr
    }

    array set APatchAttrData {
	Mode 1
	DisplayMode 0
	NPInfoBall "N/A"
	Knot-Type_U 0
	Knot-Type_V 0
	BoundaryNames { "U0" "U1" "V0" "V1" }
	BevelsChanged 0
	CapsChanged 0
    }

 return;
}
# init_APatch
