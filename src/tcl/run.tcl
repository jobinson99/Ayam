# Ayam, a free 3D modeler for the RenderMan interface.
#
# Ayam is copyrighted 1998-2008 by Randolf Schultz
# (randolf.schultz@gmail.com) and others.
#
# All rights reserved.
#
# See the file License for details.

# run.tcl - run tools
# runRenderer and auxiliary procs (runGetStdout, runGetStderr) are
# modified versions of procedures posted by
# MarK Stucky (mstucky5@home.com)
# in comp.lang.tcl (MID:"KgYT5.74231$P82.8003950@news1.rdc1.ct.home.com")


###################################
# runTool:
# execute a tool, asking for arguments
# argvars:    list of global varnames
# argstrings: labels for the argument entry fields
# command:    the command that calls the tool, where
#              "%0" will be substituted with first argument etc.
# title: title string to display in dialog window decoration
# advargs: link to documentation (html page and anchor, or just anchor)
#
# example:
# runTool splitCurveu {"Split at u:"} "splitCurve %0" "Split"
proc runTool { argvars argstrings command title {advargs ""} } {
    global ay ayprefs aymainshortcuts

    winAutoFocusOff
    set oldFocus [focus]
    set w .rtw
    winDialog $w $title

    set f [frame $w.f1]
    pack $f -in $w -side top -fill x

    set okscript ""
    append okscript "set command \"$command\";"

    if { $ayprefs(FixDialogTitles) } {
	label $f.l -text $title
	pack $f.l -in $f -side top -fill x
    }

    set naml 0
    set index 0
    foreach i $argvars {

	# is $i an array variable?
	set bri [string first "(" $i]
	if { $bri > -1 } {
	    set isarray 1
	    set j [string range $i 0 [expr $bri - 1]]
	    global $j
	} else {
	    set isarray 0
	}

	# check variable
	if { ![info exists ::$i] } {
	    set ::$i 0
	}

	# create GUI
	if { $ayprefs(SimpleToolGUI) } {
	    set f2 [frame $f.f$index]

	    label $f2.l -text [lindex $argstrings $index] -width 14
	    entry $f2.e -width 14
	    eval [subst "bindtags $f2.e \{$f2.e Entry all\}"]
	    bind $f2.e <Key-Escape> ".rtw.f2.bca invoke;break"
	    bind $f2.e <Key-Return> ".rtw.f2.bok invoke;break"
	    catch {bind $f2.e <Key-KP_Enter> ".rtw.f2.bok invoke;break"}
	    bind $f2.e <Double-ButtonPress-1> \
		"$f2.e selection range 0 end; break"
	    uie_fixEntry $f2.e

	    # fill current value of $i into the entry
	    eval "set vali \$::$i"
	    if { $vali != "" } {
		$f2.e insert 0 $vali
	    }

	    pack $f2.l -in $f2 -padx 2 -pady 2 -side left -expand no -fill x
	    pack $f2.e -in $f2 -padx 2 -pady 2 -side left -expand yes -fill x
	    pack $f2 -in $w.f1 -side top -expand yes -fill x

	    # and simultaneously build up a script
	    if { $isarray } {
		append okscript "global $j; set $i \[$f2.e get\]; update;"
	    } else {
		append okscript "global $i; set $i \[$f2.e get\]; update;"
	    }
	    append okscript \
		"regsub -all \"%$index\" \$command \[$f2.e get\] command;"

	} else {
	    # use standard UI elements
	    set ay(bok) $w.f2.bok
	    set ay(bca) $w.f2.bca
	    if { ($ayprefs(UndoLevels) > 0) &&
		 ([string first "undo save" $command] != -1) } {
		# disable instant apply if preview is available
		set ay(iapplydisable) 1
	    }
	    global ToolParams
	    set nami [lindex $argstrings $index]
	    set l [string length $nami]
	    if { $l > $naml } {
		set naml $l
	    }
	    set nam [string range $nami 0 end-1]
	    regsub -all "\[^\[:alnum:\]^-\]" $nam "_" nami

	    eval "set vali \$::$i"
	    set ToolParams($nami) $vali
	    while { 1 } {
		if { $vali != "" } {
		    if { [string is integer $vali] } {
			set l ""
			set il $i
			if { $isarray } {
			    set il [string map {")" "_l)"} $il]
			} else {
			    append il _l
			}
			catch {eval "set l \$::${il}"}
			if { $l != "" } {
			    addMenu $w.f1 ToolParams $nam $l
			} else {
			    addParam $w.f1 ToolParams $nam
			}
			break;
		    }
		    if { [string is double $vali] } {
			addParam $w.f1 ToolParams $nam
			break;
		    }
		    if { [string is boolean $vali] } {
			addCheck $w.f1 ToolParams $nam {true false}
			break;
		    }
		}
		addString $w.f1 ToolParams $nam
		set l [string length $vali]
		if { $l > 64 } { set l 64 }
		$w.f1.f$nami.e conf -width $l

		break;
	    }
	    # while

	    # and simultaneously build up a script
	    if { $isarray } {
	       append okscript "global $j; set $i \$ToolParams($nami); update;"
	    } else {
	       append okscript "global $i; set $i \$ToolParams($nami); update;"
	    }
	    append okscript \
	       "regsub -all \"%$index\" \$command \$ToolParams($nami) command;"
	}
	incr index
    }
    # foreach

    if { $naml > 0 } {
	uie_setLabelWidth $w.f1 $naml
    }

    # complete the script
    append okscript "set ay(lasttool) \$command;"
    append okscript "eval \"\$command\";"
    set ay(tguiokscript) $okscript

    set have_preview_button 0
    if { ($ayprefs(UndoLevels) > 0) &&
	 ([string first "undo save" $okscript] != -1) } {
	set txt "Preview"
	if { $ay(ws) == "Windows" } {
	    append txt "  "
	}
	addCommand $w.f1 c1 $txt _runPreview
	set have_preview_button 1
    }
    set ay(havepreview) 0

    set f [frame $w.f2]
    button $f.bok -text "Ok" -pady $ay(pady) -width 5\
	-command "_runOk $w $oldFocus $have_preview_button"

    button $f.bca -text "Cancel" -pady $ay(pady) -width 5\
	-command "_runCancel $w $oldFocus $have_preview_button"

    pack $f.bok $f.bca -in $f -side left -fill x -expand yes
    pack $f -in $w -side top -anchor n -fill x -expand yes

    bind $w <Key-Return> "$f.bok invoke"
    catch {bind $w <Key-KP_Enter> "$f.bok invoke"}
    bind $w <Escape> "$f.bca invoke"

    if { $advargs != "" } {
	if { [llength $advargs] > 1 } {
	    shortcut_addcshelp $w [lindex $advargs 0] [lindex $advargs 1]
	} else {
	    shortcut_addcshelp $w ayam-5.html $advargs
	}
    }

    if { $have_preview_button } {
	set ay(runtoolbymouse) 0
	bind $f.bok <Enter> "set ay(runtoolbymouse) 1"
	bind $f.bok <Leave> "set ay(runtoolbymouse) 0"
    }

    update idletasks

    winRestoreOrCenter $w $title

    if { $have_preview_button } {
	tgui_block "User interaction restricted until tool dialog is closed!"
    } else {
	grab $w
    }

    focus $f.bok
    tkwait window $w
    # dialog closed
    undo freeze 0

#    after idle viewMouseToCurrent

    winAutoFocusOn

 return;
}
# runTool

proc _runPreview { } {
 global ay ay_error ToolParams
    undo freeze 0
    if { $ay(havepreview) } {
	undo
    }
    set ay(havepreview) 0
    set ay_error 0
    eval $ay(tguiokscript)
    if { $ay_error < 2 } {
	set ay(havepreview) 1
	.rtw.f1.fc1.b conf -image ay_previewok_img -compound right
    } else {
	.rtw.f1.fc1.b conf -image ay_previewfail_img -compound right
    }
    undo freeze 1
 return;
}

proc _runOk { w oldFocus have_preview_button } {
 global ay ToolParams

    if { $have_preview_button && $ay(runtoolbymouse) == 0 } {
	# _runOk called via keyboard from property GUI element and _not_
	# from button via mouse click...
	_runPreview
	return;
    }
    undo freeze 0
    if { ! $ay(havepreview) } {
	eval $ay(tguiokscript)
    }
    if { $have_preview_button } {
	after idle tgui_unblock
    } else {
	grab release $w
    }
    restoreFocus $oldFocus
    destroy $w
 return;
}

proc _runCancel { w oldFocus have_preview_button } {
 global ay
    undo freeze 0
    if { $ay(havepreview) } {
	undo;rV
    }
    if { $have_preview_button } {
	after idle tgui_unblock
    } else {
	grab release $w
    }
    restoreFocus $oldFocus
    destroy $w
 return;
}


# updateRenderProgress:
# helper for runGetOutput, updates the progress widget of
# the corresponding rendering window, also computes
# the elapsed/remaining rendering time
proc updateRenderProgress { num percent } {
    global ay ayprefs
    update

    if { [catch { set str [string index [.render${num}.f2.la cget -text] 0] }]
	 != 0 } {
	return;
    }

    if { $str == "~" } {
	catch { SetProgress .render${num}.f1.p $percent }
	set cur [clock seconds]
	set start $ay(rstarttime${num})
	set fulltime [expr ($cur-$start)*100.0/$percent]
	set togo [expr (100.0-$percent)*$fulltime/100.0]
	set hours [expr int(floor($togo/3600))]
	set mins [expr int(floor(($togo-($hours*3600))/60))]
	set secs [expr int(round($togo-($hours*3600)-($mins*60)))]
	set string [format "~ %d:%02d:%02d to go" $hours $mins $secs]
	catch { .render${num}.f2.la configure -text $string }
	if { $percent >= 100 } {
	    set fulltime [expr ($cur-$start)]
	    set hours [expr int(floor($fulltime/3600))]
	    set mins [expr int(floor(($fulltime-($hours*3600))/60))]
	    set secs [expr int(round($fulltime-($hours*3600)-\
					 ($mins*60)))]
	    set string [format "%d:%02d:%02d elapsed" \
			    $hours $mins $secs]
	    catch { .render${num}.f2.la configure -text $string }
	    if {$ay(renderbeep${num})} {bell}

	    if { $ayprefs(AutoCloseUI) == 1 } {
		after idle ".render${num}.bca invoke"
	    }

	}
	# if
    }
    #if
    update
 return;
}
# updateRenderProgress


###################################
# runGetOutput:
# get and parse the renderer output from stdout/stderr
proc runGetOutput { iserr num cmd template channel } {
    global ayprefs ay
    if { [eof $channel] } {
	if { $iserr } {
	    # program completed => invoke cancel button to properly
	    # clean up all processes and destroy the RenderGUI window
	    catch { .render${num}.bca invoke }
	} else {
	    # chan is stdout
	    catch { close $channel }
	}
    } else {
        set xx [gets $channel]
	set percent 0
	if { $template != "" } {
	    if { [string first "regexp" $template] == 0 } {
		regsub "string" $template "{$xx}" template2
		eval $template2
	    } else {
		scan $xx $template percent
	    }
	}
	if { $percent != 0 } {
	    updateRenderProgress $num $percent
	} else {
	    if { $iserr } {
		ayError 2 $cmd $xx
	    }
	}
    }
    # if eof
}
# runGetOutput


###################################
# runRenderer:
#
proc runRenderer { win cmd template } {
    global ay ayprefs

    winAutoFocusOff

    if { [string first ".view" $win] != 0 } {
	# $w does not start with ".view" => view is internal
	# => center render gui dialog on main window
	set win .
    }

    if { $ayprefs(Kill) != "" } {
	set kill $ayprefs(Kill)
    } else {
	set kill "kill"
    }
    if { $ayprefs(Cat) != "" } {
	set cat $ayprefs(Cat)
    } else {
	set cat "cat"
    }

    set wait $ayprefs(Wait)

    incr ay(rnum)

    set ay(rstarttime$ay(rnum)) [clock seconds]

    set ioPipe [open "|$cat" r+]
    fconfigure $ioPipe -buffering none -blocking 0
    fileevent $ioPipe readable "\
     runGetOutput 1 $ay(rnum) [lindex $cmd 0] \"$template\" $ioPipe"

    set ioFid [open "|$cmd 2>@$ioPipe" r+]

    fconfigure $ioFid -buffering none -blocking 0
    fileevent $ioFid readable "\
     runGetOutput 0 $ay(rnum) [lindex $cmd 0] \"$template\" $ioFid"

    global pids
    set pids [list [pid $ioPipe]]
    lappend pids [pid $ioFid]

    set oldFocus [focus]

    set w .render$ay(rnum)
    set t "Render_$ay(rnum)"
    winDialog $w $t

    if { $ayprefs(FixDialogTitles) == 1 } {
	pack [frame $w.fl] -in $w -side top
	pack [label $w.fl.l -text $t] -in $w.fl -side left -fill x -expand yes
    }

    set f [frame $w.f1 -border 10]

    Progress $f.p
    pack $f.p -in $f -side top -fill x -expand yes
    pack $f -in $w -side top -fill x

    set f [frame $w.f2]
    label $f.la -text "~ 0:00:00 to go"
    pack $f.la -in $f -side left -fill x -expand yes

    set ay(renderbeep$ay(rnum)) 0
    set hl [expr round($ay(GuiScale))]
    global tcl_platform
    if { $tcl_platform(platform) == "windows" } {
	checkbutton $f.cb -image ay_Empty66_img\
	    -variable ay(renderbeep$ay(rnum))\
	    -bd 1 -indicatoron 0 -selectcolor #b03060\
	    -highlightthickness $hl
	pack $f.cb -in $f -side left -pady 2 -expand yes -anchor center
    } else {
	checkbutton $f.cb -variable ay(renderbeep$ay(rnum))\
	    -highlightthickness $hl
	pack $f.cb -in $f -side left -fill x -expand yes
    }
    balloon_set $f.cb "Beep when finished."
    pack $f -in $w -side top -fill x

    button $w.bca -text "Cancel!" -width 16 -command "\
	    catch \{ fileevent $ioPipe readable \"\" \};
	    catch \{ fileevent $ioFid readable \"\" \};
            catch \{ close $ioPipe \};
            catch \{ close $ioFid \};
	    foreach i {$pids} {
	     if { \"$kill\" == \"w32kill\" } {
	      catch \{ w32kill \$i; \} result } else {
	      catch \{ exec $kill \$i \} result;
	     };
	     if { \$result != \"\" } {
	      if { \"$wait\" != \"\" } {
	        catch \{ $wait \$i \};
              };
             };
            };
	    restoreFocus $oldFocus;
            winAutoFocusOn;
	    destroy $w"
    pack $w.bca -in $w -side bottom -anchor s -fill x -expand yes

    # center dialog on screen or inside the view (if external)
    winCenter $w $win

    # auto raise window, when obscured
    bind $w <Visibility> {
	global ay
	# are we obscured?
	if { "%s" == "VisibilityPartiallyObscured" ||\
		"%s" == "VisibilityFullyObscured" } {
	    # yes: try to raise the window, but just one time
	    raise [winfo toplevel %W]
	    bind %W <Visibility> ""
	}
    }

    # Esc-Key == Cancel button
    bind $w <Escape> "$w.bca invoke"

    wm protocol $w WM_DELETE_WINDOW "$w.bca invoke;break"

    focus $w

 return;
}
# runRenderer

# helper for runBG below
proc runBGHandler { chan finish } {
    read $chan
    if {[eof $chan]} {
	close $chan
        eval $finish
    }
}

# runBG:
#  run a shell command in the background and when finished run a Tcl command
proc runBG { cmd { finish "" } } {
    set chan [open "| $cmd" r]
    fconfigure $chan -blocking false
    fileevent $chan readable [list runBGHandler $chan $finish]
 return;
}
# runBG
