# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2019 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# closet.tcl - add button to close the tree

image create photo ay_CloseT_img -format GIF -data {\
R0lGODlhEgASAIAAAAAAAAAAACH5BAEKAAEALAAAAAASABIAAAIljI+py+0G
opxRAXdTZvt0CzZfkFFlaEIhty7jiLyYZVLPjedLAQA7}

if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_CloseT_img
}

button .fu.fMain.ctb -command "selOb;cS;tree_gotop;tree_collapse $ay(tree);"\
	-image ay_CloseT_img -rel flat\
	-overrel raised -borderwidth 1

_placeTreeIcon .fu.fMain.ctb

return;
# EOF
