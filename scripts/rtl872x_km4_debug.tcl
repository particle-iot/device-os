
transport select swd
adapter speed 1000

source [find target/swj-dp.tcl]

if { [info exists CHIPNAME] } {
	set _CHIPNAME $CHIPNAME
} else {
	set _CHIPNAME rtl872x
}

if { [info exists WORKAREASIZE] } {
   set _WORKAREASIZE $WORKAREASIZE
} else {
   set _WORKAREASIZE 0x4000
}

if { [info exists CPUTAPID] } {
	set _CPUTAPID $CPUTAPID
} else {
	set _CPUTAPID 0x6ba02477
}

swj_newdap $_CHIPNAME cpu -irlen 4 -expected-id $_CPUTAPID
dap create $_CHIPNAME.dap -chain-position $_CHIPNAME.cpu

set _TARGETNAME $_CHIPNAME.cpu
target create $_TARGETNAME cortex_m -dap $_CHIPNAME.dap -ap-num 2

$_CHIPNAME.cpu cortex_m reset_config sysresetreq
# $_CHIPNAME.cpu configure -gdb-port 3334

set RTL872x_REG_BOOT_CFG                0x480003F8

$_CHIPNAME.cpu configure -event reset-assert-post {
    global RTL872x_REG_BOOT_CFG
    echo "> KM4: reset-assert-post event"
    mww $RTL872x_REG_BOOT_CFG 0x02000201
    echo "[mdw 0x480003F8]"
}
