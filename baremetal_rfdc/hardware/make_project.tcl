# RFSoC4x2 clock + RF data converter bring-up, Vivado 2020.2
#
#   vivado -mode batch -source hardware/make_project.tcl                 (project + block design)
#   vivado -mode batch -source hardware/make_project.tcl -tclargs build  (+ bitstream + .xsa)
#
# PS (board preset: SPI0 to the clock chips, UART1) + RF data converter:
#   DAC_B = DAC tile 228 ch0, ADC_B = ADC tile 226 ch0, own tile PLLs, 491.52 MHz reference
#   from the LMX2594s, 3.93216 GSPS, I/Q with x8 interpolation / decimation, 245.76 MHz fabric.
# Copyright (c) 2026, Yijie Yu. BSD-3-Clause.

set build [expr {[llength $argv] > 0 && [lindex $argv 0] eq "build"}]

set here      [file normalize [file dirname [info script]]/..]
set proj_dir  $here/build
set bd_name   design_1

create_project clk_rfdc $proj_dir -part xczu48dr-ffvg1517-2-e -force
set_property board_part realdigital.org:rfsoc4x2:part0:1.0 [current_project]
create_bd_design $bd_name

# ---------------------------------------------------------------- PS
set ps [create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e zynq_ultra_ps_e_0]
apply_bd_automation -rule xilinx.com:bd_rule:zynq_ultra_ps_e -config {apply_board_preset "1"} $ps
set_property -dict [list \
    CONFIG.PSU__USE__M_AXI_GP0 {1} \
    CONFIG.PSU__USE__M_AXI_GP1 {0} \
    CONFIG.PSU__USE__M_AXI_GP2 {0} \
    CONFIG.PSU__FPGA_PL0_ENABLE {1} \
    CONFIG.PSU__CRL_APB__PL0_REF_CTRL__FREQMHZ {100} \
    CONFIG.PSU__DDRC__ROW_ADDR_COUNT {16} \
] $ps

# ---------------------------------------------------------------- RF data converter
set rfdc [create_bd_cell -type ip -vlnv xilinx.com:ip:usp_rf_data_converter usp_rf_data_converter_0]
set_property -dict [list \
    CONFIG.ADC0_Enable {0} \
    CONFIG.ADC_Slice00_Enable {false} \
    CONFIG.ADC_Slice01_Enable {false} \
    CONFIG.ADC2_Enable {1} \
    CONFIG.ADC2_PLL_Enable {true} \
    CONFIG.ADC2_Refclk_Freq {491.520} \
    CONFIG.ADC2_Sampling_Rate {3.93216} \
    CONFIG.ADC2_Fabric_Freq {245.760} \
    CONFIG.ADC2_Outclk_Freq {245.760} \
    CONFIG.ADC_Slice20_Enable {true} \
    CONFIG.ADC_Slice21_Enable {true} \
    CONFIG.ADC_Data_Type20 {1} \
    CONFIG.ADC_Data_Type21 {1} \
    CONFIG.ADC_Data_Width20 {2} \
    CONFIG.ADC_Data_Width21 {2} \
    CONFIG.ADC_Decimation_Mode20 {8} \
    CONFIG.ADC_Decimation_Mode21 {8} \
    CONFIG.ADC_Mixer_Type20 {2} \
    CONFIG.ADC_Mixer_Type21 {2} \
    CONFIG.ADC_Mixer_Mode20 {0} \
    CONFIG.ADC_Mixer_Mode21 {0} \
    CONFIG.ADC_NCO_Freq20 {-0.6} \
    CONFIG.DAC0_Enable {1} \
    CONFIG.DAC0_PLL_Enable {true} \
    CONFIG.DAC0_Refclk_Freq {491.520} \
    CONFIG.DAC0_Sampling_Rate {3.93216} \
    CONFIG.DAC0_Fabric_Freq {245.760} \
    CONFIG.DAC0_Outclk_Freq {245.760} \
    CONFIG.DAC_Slice00_Enable {true} \
    CONFIG.DAC_Data_Width00 {4} \
    CONFIG.DAC_Interpolation_Mode00 {8} \
    CONFIG.DAC_Mixer_Type00 {2} \
    CONFIG.DAC_Mixer_Mode00 {0} \
    CONFIG.DAC_NCO_Freq00 {0.6} \
] $rfdc

foreach p {dac0_clk adc2_clk sysref_in vout00 vin2_01} {
    make_bd_intf_pins_external [get_bd_intf_pins usp_rf_data_converter_0/$p]
}

# ---------------------------------------------------------------- clocks, resets, AXI-Lite
set clk_ps  [get_bd_pins zynq_ultra_ps_e_0/pl_clk0]
set clk_dac [get_bd_pins usp_rf_data_converter_0/clk_dac0]
set clk_adc [get_bd_pins usp_rf_data_converter_0/clk_adc2]
foreach {name clk} [list rst_ps $clk_ps rst_dac $clk_dac rst_adc $clk_adc] {
    create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset $name
    connect_bd_net $clk [get_bd_pins $name/slowest_sync_clk]
    connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_resetn0] [get_bd_pins $name/ext_reset_in]
}

connect_bd_net $clk_dac [get_bd_pins usp_rf_data_converter_0/s0_axis_aclk]
connect_bd_net [get_bd_pins rst_dac/peripheral_aresetn] [get_bd_pins usp_rf_data_converter_0/s0_axis_aresetn]
connect_bd_net $clk_adc [get_bd_pins usp_rf_data_converter_0/m2_axis_aclk]
connect_bd_net [get_bd_pins rst_adc/peripheral_aresetn] [get_bd_pins usp_rf_data_converter_0/m2_axis_aresetn]

set ic [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect lite_interconnect]
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {1}] $ic
connect_bd_intf_net [get_bd_intf_pins zynq_ultra_ps_e_0/M_AXI_HPM0_FPD] [get_bd_intf_pins lite_interconnect/S00_AXI]
connect_bd_intf_net [get_bd_intf_pins lite_interconnect/M00_AXI] [get_bd_intf_pins usp_rf_data_converter_0/s_axi]
connect_bd_net $clk_ps [get_bd_pins zynq_ultra_ps_e_0/maxihpm0_fpd_aclk] [get_bd_pins lite_interconnect/ACLK] \
    [get_bd_pins lite_interconnect/S00_ACLK] [get_bd_pins lite_interconnect/M00_ACLK] \
    [get_bd_pins usp_rf_data_converter_0/s_axi_aclk]
connect_bd_net [get_bd_pins rst_ps/interconnect_aresetn] [get_bd_pins lite_interconnect/ARESETN]
connect_bd_net [get_bd_pins rst_ps/peripheral_aresetn] [get_bd_pins lite_interconnect/S00_ARESETN] \
    [get_bd_pins lite_interconnect/M00_ARESETN] [get_bd_pins usp_rf_data_converter_0/s_axi_aresetn]

assign_bd_address -offset 0xA0000000 -range 256K \
    -target_address_space [get_bd_addr_spaces zynq_ultra_ps_e_0/Data] \
    [get_bd_addr_segs usp_rf_data_converter_0/s_axi/Reg]

validate_bd_design
save_bd_design
make_wrapper -files [get_files $bd_name.bd] -top -import
set_property top ${bd_name}_wrapper [current_fileset]
update_compile_order -fileset sources_1

if {$build} {
    launch_runs impl_1 -to_step write_bitstream -jobs 8
    wait_on_run impl_1
    if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
        error "implementation failed"
    }
    write_hw_platform -fixed -include_bit -force -file $here/build/${bd_name}_wrapper.xsa
}
