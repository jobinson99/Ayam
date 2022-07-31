# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2021 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# dskin.tcl - DSkin objects Tcl code

proc init_DSkin { } {
    global ay DSkin_props DSkinAttr DSkinAttrData

    set DSkin_props [list Transformations Attributes Material Tags Bevels\
			  Caps DSkinAttr]

    array set DSkinAttr {
	arr   DSkinAttrData
	sproc {setProp undo}
	gproc ""
	w     fDSkinAttr
    }

    array set DSkinAttrData {
	DisplayMode 1
	NPInfoBall "n/a"
	BoundaryNames { "U0" "U1" "V0" "V1" }
	BoundaryIDs { 0 1 2 3 }
    }

    set w [frame $ay(pca).$DSkinAttr(w)]
    set a $DSkinAttr(arr)

    addVSpace $w s1 2
    addCheck $w $a WatchCorners
    addParam $w $a Order_U
    addParam $w $a Order_V

    addVSpace $w s2 4
    addParam $w $a Tolerance
    addMenu $w $a DisplayMode $ay(npdisplaymodes)

    addVSpace $w s3 4
    addText $w $a "Created NURBS Patch:"
    addInfo $w $a NPInfo

    rename init_DSkin ""

 return;
}
# init_DSkin
