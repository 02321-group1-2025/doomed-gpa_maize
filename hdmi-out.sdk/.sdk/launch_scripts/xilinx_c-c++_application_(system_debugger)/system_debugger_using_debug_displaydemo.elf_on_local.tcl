connect -url tcp:127.0.0.1:3121
source C:/DTU/semester_3/hw_sw/3_uger/hdmi-out/hdmi-out.sdk/hdmi_out_wrapper_hw_platform_0/ps7_init.tcl
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279787755A"} -index 0
loadhw -hw C:/DTU/semester_3/hw_sw/3_uger/hdmi-out/hdmi-out.sdk/hdmi_out_wrapper_hw_platform_0/system.hdf -mem-ranges [list {0x40000000 0xbfffffff}]
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*" && jtag_cable_name =~ "Digilent Zybo 210279787755A"} -index 0
stop
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279787755A"} -index 0
rst -processor
targets -set -nocase -filter {name =~ "ARM*#0" && jtag_cable_name =~ "Digilent Zybo 210279787755A"} -index 0
dow C:/DTU/semester_3/hw_sw/3_uger/hdmi-out/hdmi-out.sdk/displaydemo/Debug/displaydemo.elf
configparams force-mem-access 0
bpadd -addr &main
