# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2021 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# shufup.tcl - add button to shuffle the current selection up

image create photo ay_ShufUp_img -format GIF -data {\
R0lGODlhEgASAIAAAAAAAAAAACH5BAEKAAEALAAAAAASABIAAAIojI+pywbN
3gEU0nstyk1GhXlOtUVY2YFnIjoau5FBO4cyNNL4zvdKAQA7}

if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_ShufUp_img
}

button .fu.fMain.sub -command "selMUD 1"\
	-image ay_ShufUp_img -rel flat\
	-overrel raised -borderwidth 1

_placeTreeIcon .fu.fMain.sub

return;
# EOF
