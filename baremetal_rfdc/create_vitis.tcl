# Vitis 2020.2 workspace for the clock + RFDC bring-up.
#   xsct create_vitis.tcl [path/to/design_1_wrapper.xsa]
# Copyright (c) 2026, Yijie Yu. BSD-3-Clause.

set here [file normalize [file dirname [info script]]]
set xsa  [expr {[llength $argv] > 0 ? [file normalize [lindex $argv 0]] : "$here/build/design_1_wrapper.xsa"}]
set ws   $here/vitis_ws

file delete -force $ws
setws $ws

platform create -name clk_rfdc_pf -hw $xsa -proc psu_cortexa53_0 -os standalone -out $ws
platform generate

app create -name clk_rfdc -platform clk_rfdc_pf -domain standalone_domain -template "Empty Application"
importsources -name clk_rfdc -path $here/src
app config -name clk_rfdc -add libraries metal

# app build generates Debug/makefile; in batch mode it may drop the xsct channel before
# linking, so finish with make
catch {app build -name clk_rfdc}
set elf $ws/clk_rfdc/Debug/clk_rfdc.elf
if {![file exists $elf]} {
    set vitis $::env(XILINX_VITIS)
    set ::env(PATH) "$vitis/gnu/aarch64/nt/aarch64-none/bin;$vitis/gnuwin/bin;$::env(PATH)"
    exec make -C $ws/clk_rfdc/Debug all >@ stdout 2>@ stderr
}
puts "ELF: $elf"
