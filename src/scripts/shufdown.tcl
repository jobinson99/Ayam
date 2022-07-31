# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2021 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# shufdown.tcl - add button to shuffle the current selection down

image create photo ay_ShufDown_img -format GIF -data {\
R0lGODlhEgASAIAAAAAAAAAAACH5BAEKAAEALAAAAAASABIAAAInjI+pywbN
3gEU0nstyk1GhXkTqCUiaWKbOkbVtnpnEHLQ9N76zkMFADs=}

if { $ay(GuiScale) != 1.0 } {
    _zoomImage ay_ShufDown_img
}

button .fu.fMain.sdb -command "selMUD 0"\
	-image ay_ShufDown_img -rel flat\
	-overrel raised -borderwidth 1

_placeTreeIcon .fu.fMain.sdb

return;
# EOF
