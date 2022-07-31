# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2019 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# opent.tcl - add button to open the tree

image create photo ay_OpenT_img -format GIF -data {\
R0lGODlhEgASAIABAAAAAP///yH5BAEKAAEALAAAAAASABIAAAIqjI+py+0G
opxRAXdTZvv06lkICIkBlXXlSa3j+JlQrGHyorLo9PT+vygAADs=}

if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_OpenT_img
}

button .fu.fMain.otb -command "selOb;cS;tree_gotop;tree_expand $ay(tree);"\
	-image ay_OpenT_img -rel flat\
	-overrel raised -borderwidth 1

_placeTreeIcon .fu.fMain.otb

return;
# EOF
