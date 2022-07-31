# Ayam, save array: HDiskAttrData
# hdisk.tcl: example script for an Ayam Script object;
# this script wants Script object type "Create" and creates
# a disk with a hole; it also has a property GUI, just
# add a tag "NP HDiskAttr" to the Script object to see it
if { ![info exists ::HDiskAttrData] } {
    array set ::HDiskAttrData {
	ThetaMax 360.0
	RMin 0.5
	RMax 1
	SP {ThetaMax RMin RMax}
    }
}
if { ![info exists ::HDiskAttrGUI] } {
    set w [addPropertyGUI HDiskAttr "" ""]
    addVSpace $w s1 2
    addParam $w HDiskAttrData ThetaMax
    addParam $w HDiskAttrData RMin
    addParam $w HDiskAttrData RMax
}
crtOb Hyperboloid; sL
getProp
set HyperbAttrData(ThetaMax) $HDiskAttrData(ThetaMax)
set HyperbAttrData(P1_X) $HDiskAttrData(RMin)
set HyperbAttrData(P1_Y) 0
set HyperbAttrData(P1_Z) 0
set HyperbAttrData(P2_X) $HDiskAttrData(RMax)
set HyperbAttrData(P2_Y) 0
set HyperbAttrData(P2_Z) 0
setProp

# hdisk.tcl
