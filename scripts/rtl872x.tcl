#
# OpenOCD script for RTL872x
#

source [find target/swj-dp.tcl]

if { [info exists CHIPNAME] } {
    set _CHIPNAME $CHIPNAME
} else {
    set _CHIPNAME rtl872x
}

if { [info exists ENDIAN] } {
    set _ENDIAN $ENDIAN
} else {
    set _ENDIAN little
}

if { [info exists WORKAREASIZE] } {
    set _WORKAREASIZE $WORKAREASIZE
} else {
    set _WORKAREASIZE 0x800
}

if { [info exists CPUTAPID] } {
    set _CPUTAPID $CPUTAPID
} else {
    set _CPUTAPID 0x6ba02477
}

swj_newdap $_CHIPNAME cpu -irlen 4 -expected-id $_CPUTAPID
dap create $_CHIPNAME.dap -chain-position $_CHIPNAME.cpu

set _TARGETNAME $_CHIPNAME.cpu
target create $_TARGETNAME cortex_m -endian $_ENDIAN -dap $_CHIPNAME.dap

$_TARGETNAME configure -work-area-phys 0x00082000 -work-area-size $_WORKAREASIZE -work-area-backup 0

adapter_khz 4000
adapter_nsrst_delay 100

if {![using_hla]} {
    cortex_m reset_config sysresetreq
}

#
# Flash loader for RTL872x
#
# PC msg format:  | Command(4Byte) | data |
# MCU msg format: | Result(4Byte)  | data |
#
set PC_MSG_CMD_OFFSET                   0
set PC_MSG_DATA_OFFSET                  4
set MCU_MSG_RESULT_OFFSET               0
set MCU_MSG_DATA_OFFSET                 4

set RESULT_OK                           0x00
set RESULT_ERROR                        0x01

set CMD_FLASH_READ_ID                   0x10  ;# | CMD(4Byte)|
set CMD_FLASH_ERASE_ALL                 0x11  ;# | CMD(4Byte)|
set CMD_FLASH_ERASE_SECTOR              0x12  ;# | CMD(4Byte)| Start Sector Number(4Bytes)  | Number of sectors to erase |
set CMD_FLASH_READ                      0x13  ;# | CMD(4Byte)|    Address(4Bytes)    | Length(4Bytes) |
set CMD_FLASH_WRITE                     0x14  ;# | CMD(4Byte)|    Address(4Bytes)    | Length(4Bytes) | reserved  | Data(NBytes)  |
set CMD_FLASH_VERIFY                    0x15  ;# | CMD(4Byte)|    Address(4Bytes)    | Length(4Bytes) |    CRC32      |

set RTL872x_REG_PERI_ON_BASE            0x48000000
set RTL872x_REG_BOOT_CFG                0x480003F8
set RTL872x_REG_UNKNOWN_STOP_KM4        0x4800021C  ; # bit3 = 0, bit24 = 0
set RTL872x_REG_UNKNOWN_HW_RESET        0x48000054  ; # Enable HW reset when download finish, bit1 = 1

set FLASH_SECTOR_SIZE                   4096
set FLASH_MMU_START_ADDR                0x08000000

set MSG_BUF_SIZE                        1200
set pc_msg_avail_mem_addr               0x00082874
set mcu_msg_avail_mem_addr              0x00082870
set pc_msg_data_mem_addr                0x00082880
set mcu_msg_data_mem_addr               0x000823c0

set MSG_FLASH_DATA_BUF_SIZE             [expr {$MSG_BUF_SIZE / 1024 * 1024}]

set rtl872x_ready                       0


proc openocd_read_register {reg} {
    set value ""
    mem2array value 32 $reg 1
    return $value(0)
}

proc openocd_write_register {reg value} {
    mww $reg $value
}

proc openocd_print_memory {address length} {
    set value ""
    mem2array value 8 $address $length
    echo ""
    echo "---------------------------------------------------------"
    echo "  Offset| 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"
    echo "---------------------------------------------------------"
    for {set i 0} {$i < $length} {incr i} {
        if {$i % 16 == 0} {
            echo -n "[format {%08X} [expr {$address + $i}]]| "
        }

        echo -n "[format %02X $value($i)] "
        if {[expr $i > 0 && ($i+1) % 16 == 0] || [expr $i == $length - 1]} {
            echo ""
        }
    }
    echo "---------------------------------------------------------"
    echo ""
}

proc progress_bar {cur tot} {
    # if you don't want to redraw all the time, uncomment and change ferquency
    #if {$cur % ($tot/300)} { return }
    # set to total width of progress bar
    set total 76
    set half 0 ;#[expr {$total/2}]
    set percent [expr {100.*$cur/$tot}]
    set val "[format "%2.0f%%" $percent]"
    set str "\r  |[string repeat = [
                expr {round($percent*$total/100)}]][
                        string repeat { } [expr {$total-round($percent*$total/100)}]]|"
    set str "[string range $str 0 $half]$val[string range $str [expr {$half+[string length $val]-1}] end]"
    puts -nonewline stderr $str
}

proc rtl872x_init {} {
    global rtl872x_ready
    global pc_msg_avail_mem_addr
    global mcu_msg_avail_mem_addr
    global RTL872x_REG_BOOT_CFG
    global RTL872x_REG_UNKNOWN_STOP_KM4
    global rtl872x_flash_loader

    if {[expr {$rtl872x_ready == 0}]} {
        echo "initializing rtl872x"

        reset halt

        # load_image /Users/yuting/Git/mcu_prj/realtek_ambd_sdk/project/realtek_amebaD_va0_example/GCC-RELEASE/project_lp/asdk/gnu_utility/gnu_script/acut_loader/flash_loader_ram_1.bin 0x00082000
        array2mem rtl872x_flash_loader 32 0x00082000 [array size rtl872x_flash_loader]

        # Set rom boot BIT to flash loader
        set reg_val [openocd_read_register $RTL872x_REG_BOOT_CFG]
        set reg_val [expr $reg_val & ~(0xFFFF << 16)]
        set reg_val [expr $reg_val | (0x01 << 26)]
        #echo "boot cfg reg write: [format {0x%08X} $reg_val]"
        mww $RTL872x_REG_BOOT_CFG $reg_val

        # Stop KM4 when downloading flash
        set reg_val [openocd_read_register $RTL872x_REG_UNKNOWN_STOP_KM4]
        set reg_val [expr $reg_val & ~(1 << 3)]
        set reg_val [expr $reg_val & ~(1 << 24)]
        mww $RTL872x_REG_UNKNOWN_STOP_KM4 $reg_val

        # Reset communication register
        openocd_write_register $pc_msg_avail_mem_addr 0
        openocd_write_register $mcu_msg_avail_mem_addr 0

        resume 0x00082000

        set rtl872x_ready 1
    }
}

proc wait_response {timeout} {
    global mcu_msg_avail_mem_addr
    global mcu_msg_data_mem_addr
    set startTime [clock milliseconds]
    set newline 0
    # echo "-mcu msg avail: [openocd_read_register [expr $mcu_msg_avail_mem_addr]]"
    while {[expr [openocd_read_register [expr $mcu_msg_avail_mem_addr]] == 0]} {
        if {$timeout && [expr [clock milliseconds] - $startTime] > $timeout} {
            return 0
        }
    }
    # echo "+mcu msg avail: [openocd_read_register [expr $mcu_msg_avail_mem_addr]]"
    openocd_write_register $mcu_msg_avail_mem_addr 0
}

proc rtl872x_flash_read_id {} {
    global pc_msg_avail_mem_addr
    global mcu_msg_avail_mem_addr
    global pc_msg_data_mem_addr
    global mcu_msg_data_mem_addr
    global CMD_FLASH_READ_ID

    rtl872x_init
    halt
    mww [expr {$pc_msg_data_mem_addr + 0x00}] $CMD_FLASH_READ_ID
    mww $pc_msg_avail_mem_addr 1
    resume
    wait_response 0
    set id [openocd_read_register [expr {$mcu_msg_data_mem_addr + 0x04}]]
    echo "RTL872x flash id: $id"
}

proc rtl872x_flash_dump {address length} {
    global FLASH_MMU_START_ADDR
    rtl872x_init
    set address [expr $address + $FLASH_MMU_START_ADDR]
    openocd_print_memory $address $length
}

proc rtl872x_flash_erase_all {} {
    global pc_msg_avail_mem_addr
    global mcu_msg_avail_mem_addr
    global pc_msg_data_mem_addr
    global mcu_msg_data_mem_addr
    global CMD_FLASH_ERASE_ALL

    rtl872x_init
    halt
    mww [expr {$pc_msg_data_mem_addr + 0x00}] $CMD_FLASH_ERASE_ALL
    mww $pc_msg_avail_mem_addr 1
    resume
    wait_response 0
    echo "RTL872x mass erase flash"
}

proc rtl872x_flash_write_bin {file address} {
    # Enable auto_erase and auto_verify 
    rtl872x_flash_write_bin_ext $file $address 1 1
}

proc rtl872x_flash_write_bin_ext {file address auto_erase auto_verify} {
    global pc_msg_avail_mem_addr
    global mcu_msg_avail_mem_addr
    global pc_msg_data_mem_addr
    global mcu_msg_data_mem_addr
    global CMD_FLASH_WRITE
    global CMD_FLASH_ERASE_SECTOR
    global CMD_FLASH_VERIFY
    global FLASH_SECTOR_SIZE
    global MSG_FLASH_DATA_BUF_SIZE
    global RESULT_OK
    global RESULT_ERROR

    rtl872x_init
    set size [file size $file]

    echo ""
    echo "Downloading file: [file tail $file], Size: $size, Address: $address"

    if {[expr $address % $FLASH_SECTOR_SIZE != 0]} {
        error "address must be sector-aligned"
    }

    # Erase sectors
    set startTime [clock milliseconds]
    if {$auto_erase} {
        echo "Erase flash:"
        set sector_start_num [expr $address / $FLASH_SECTOR_SIZE]
        set sector_count [expr [expr $size + $FLASH_SECTOR_SIZE - 1] / $FLASH_SECTOR_SIZE]

        mww [expr {$pc_msg_data_mem_addr + 0x00}] $CMD_FLASH_ERASE_SECTOR
        mww [expr {$pc_msg_data_mem_addr + 0x04}] $sector_start_num
        mww [expr {$pc_msg_data_mem_addr + 0x08}] $sector_count
        mww $pc_msg_avail_mem_addr 1
    
        set fake_progress 0
        while {[wait_response 500] == 0} {
            sleep 500
            set fake_progress [expr $fake_progress + 5]
            progress_bar $fake_progress 100
        }
        progress_bar 100 100
        echo "\nO.K. Erase time: [format "%.3f" [expr [expr [clock milliseconds] - $startTime] / 1000.0]]s"
    }

    # Write binary file
    # The load_image command has a limitation for the load file.
    # On Realtek MCU, the RAM address stats at 0x80000, which allows us to load a up-to-512KB file.
    # If the file is larger than 512KB, we'll split it into sevral 512KB pieces.
    set tmp_file_path "/tmp"
    set file_counts 0
    set f [open $file r]
    fconfigure $f -translation binary
    for {set i 0} {$i < $size} {set i [expr $i + [expr 512 * 1024]]} {
        set bin_data [read $f [expr 512 * 1024]]
        set f_tmp [open "$tmp_file_path/op_file_$file_counts.bin" w]
        puts $f_tmp $bin_data
        close $f_tmp
        incr file_counts
    }
    close $f

    set startTime [clock milliseconds]
    echo "Program flash: $file_counts files in total"
    for {set file_num 0} {$file_num < $file_counts} {incr file_num} {
        set op_file "$tmp_file_path/op_file_$file_num.bin"
        set op_file_size [expr [file size $op_file] - 1] ;# file read command will read an extra newline char
        set op_file_address [expr $address + [expr 512 * 1024 * $file_num]]
        # echo "file: $tmp_file_path/op_file_$file_num.bin, size: $op_file_size, address: [format "%X" $op_file_address]"
        for {set i 0} {$i < $op_file_size} {set i [expr $i + $MSG_FLASH_DATA_BUF_SIZE]} {
            set write_size [expr [expr $op_file_size - $i > $MSG_FLASH_DATA_BUF_SIZE] ? $MSG_FLASH_DATA_BUF_SIZE : [expr $op_file_size - $i]]
            mww [expr {$pc_msg_data_mem_addr + 0x00}] $CMD_FLASH_WRITE
            mww [expr {$pc_msg_data_mem_addr + 0x04}] [expr $op_file_address + $i]
            mww [expr {$pc_msg_data_mem_addr + 0x08}] $write_size
            load_image $op_file [expr $pc_msg_data_mem_addr + 0x10 - $i] bin [expr $pc_msg_data_mem_addr + 0x10] $write_size 
            # echo "write size: $write_size, offset: $i, address: [expr $op_file_address + $i]"
            progress_bar $i $op_file_size
            mww $pc_msg_avail_mem_addr 1
            wait_response 0
        }
    }
    progress_bar 100 100
    set timelapse [expr [expr [clock milliseconds] - $startTime] / 1000.0]
    echo "\nO.K. Program time: [format "%.3f" $timelapse]s, speed: [format "%.3f" [expr [expr $size / 1024.0] / $timelapse]]KB/s"
 
    # Verify binary file
    set startTime [clock milliseconds]
    echo "Verify flash: "
    if {$auto_verify} {
        set crc32 [crc32_binary $file]
        mww [expr {$pc_msg_data_mem_addr + 0x00}] $CMD_FLASH_VERIFY
        mww [expr {$pc_msg_data_mem_addr + 0x04}] $address
        mww [expr {$pc_msg_data_mem_addr + 0x08}] $size
        mww [expr {$pc_msg_data_mem_addr + 0x0C}] $crc32
        mww $pc_msg_avail_mem_addr 1
        wait_response 0
        set timelapse [expr [expr [clock milliseconds] - $startTime] / 1000.0]
        if {[expr [openocd_read_register [expr {$mcu_msg_data_mem_addr + 0x00}]] == $RESULT_OK]} {
            echo "O.K. Verify time: [format "%.3f" [expr [expr [clock milliseconds] - $startTime] / 1000.0]]s"
        } else {
            error "FAILED!"
        }
    }
}

proc rtl872x_verify_image {file address} {
    rtl872x_init
}

proc rtl872x_flash_read {} {
    rtl872x_init
}

proc rtl872x_flash_erase_sector {} {
    rtl872x_init
}


#
# CRC32 implementation
#
# Reference: https://wiki.tcl-lang.org/page/CRC
#
# CRCTABLE is the pre-calculated lookup table provided for this
# implementation:
#
set CRCTABLE {      0x00000000 0x77073096 0xEE0E612C 0x990951BA
                    0x076DC419 0x706AF48F 0xE963A535 0x9E6495A3
                    0x0EDB8832 0x79DCB8A4 0xE0D5E91E 0x97D2D988
                    0x09B64C2B 0x7EB17CBD 0xE7B82D07 0x90BF1D91
                    0x1DB71064 0x6AB020F2 0xF3B97148 0x84BE41DE
                    0x1ADAD47D 0x6DDDE4EB 0xF4D4B551 0x83D385C7
                    0x136C9856 0x646BA8C0 0xFD62F97A 0x8A65C9EC
                    0x14015C4F 0x63066CD9 0xFA0F3D63 0x8D080DF5
                    0x3B6E20C8 0x4C69105E 0xD56041E4 0xA2677172
                    0x3C03E4D1 0x4B04D447 0xD20D85FD 0xA50AB56B
                    0x35B5A8FA 0x42B2986C 0xDBBBC9D6 0xACBCF940
                    0x32D86CE3 0x45DF5C75 0xDCD60DCF 0xABD13D59
                    0x26D930AC 0x51DE003A 0xC8D75180 0xBFD06116
                    0x21B4F4B5 0x56B3C423 0xCFBA9599 0xB8BDA50F
                    0x2802B89E 0x5F058808 0xC60CD9B2 0xB10BE924
                    0x2F6F7C87 0x58684C11 0xC1611DAB 0xB6662D3D
                    0x76DC4190 0x01DB7106 0x98D220BC 0xEFD5102A
                    0x71B18589 0x06B6B51F 0x9FBFE4A5 0xE8B8D433
                    0x7807C9A2 0x0F00F934 0x9609A88E 0xE10E9818
                    0x7F6A0DBB 0x086D3D2D 0x91646C97 0xE6635C01
                    0x6B6B51F4 0x1C6C6162 0x856530D8 0xF262004E
                    0x6C0695ED 0x1B01A57B 0x8208F4C1 0xF50FC457
                    0x65B0D9C6 0x12B7E950 0x8BBEB8EA 0xFCB9887C
                    0x62DD1DDF 0x15DA2D49 0x8CD37CF3 0xFBD44C65
                    0x4DB26158 0x3AB551CE 0xA3BC0074 0xD4BB30E2
                    0x4ADFA541 0x3DD895D7 0xA4D1C46D 0xD3D6F4FB
                    0x4369E96A 0x346ED9FC 0xAD678846 0xDA60B8D0
                    0x44042D73 0x33031DE5 0xAA0A4C5F 0xDD0D7CC9
                    0x5005713C 0x270241AA 0xBE0B1010 0xC90C2086
                    0x5768B525 0x206F85B3 0xB966D409 0xCE61E49F
                    0x5EDEF90E 0x29D9C998 0xB0D09822 0xC7D7A8B4
                    0x59B33D17 0x2EB40D81 0xB7BD5C3B 0xC0BA6CAD
                    0xEDB88320 0x9ABFB3B6 0x03B6E20C 0x74B1D29A
                    0xEAD54739 0x9DD277AF 0x04DB2615 0x73DC1683
                    0xE3630B12 0x94643B84 0x0D6D6A3E 0x7A6A5AA8
                    0xE40ECF0B 0x9309FF9D 0x0A00AE27 0x7D079EB1
                    0xF00F9344 0x8708A3D2 0x1E01F268 0x6906C2FE
                    0xF762575D 0x806567CB 0x196C3671 0x6E6B06E7
                    0xFED41B76 0x89D32BE0 0x10DA7A5A 0x67DD4ACC
                    0xF9B9DF6F 0x8EBEEFF9 0x17B7BE43 0x60B08ED5
                    0xD6D6A3E8 0xA1D1937E 0x38D8C2C4 0x4FDFF252
                    0xD1BB67F1 0xA6BC5767 0x3FB506DD 0x48B2364B
                    0xD80D2BDA 0xAF0A1B4C 0x36034AF6 0x41047A60
                    0xDF60EFC3 0xA867DF55 0x316E8EEF 0x4669BE79
                    0xCB61B38C 0xBC66831A 0x256FD2A0 0x5268E236
                    0xCC0C7795 0xBB0B4703 0x220216B9 0x5505262F
                    0xC5BA3BBE 0xB2BD0B28 0x2BB45A92 0x5CB36A04
                    0xC2D7FFA7 0xB5D0CF31 0x2CD99E8B 0x5BDEAE1D
                    0x9B64C2B0 0xEC63F226 0x756AA39C 0x026D930A
                    0x9C0906A9 0xEB0E363F 0x72076785 0x05005713
                    0x95BF4A82 0xE2B87A14 0x7BB12BAE 0x0CB61B38
                    0x92D28E9B 0xE5D5BE0D 0x7CDCEFB7 0x0BDBDF21
                    0x86D3D2D4 0xF1D4E242 0x68DDB3F8 0x1FDA836E
                    0x81BE16CD 0xF6B9265B 0x6FB077E1 0x18B74777
                    0x88085AE6 0xFF0F6A70 0x66063BCA 0x11010B5C
                    0x8F659EFF 0xF862AE69 0x616BFFD3 0x166CCF45
                    0xA00AE278 0xD70DD2EE 0x4E048354 0x3903B3C2
                    0xA7672661 0xD06016F7 0x4969474D 0x3E6E77DB
                    0xAED16A4A 0xD9D65ADC 0x40DF0B66 0x37D83BF0
                    0xA9BCAE53 0xDEBB9EC5 0x47B2CF7F 0x30B5FFE9
                    0xBDBDF21C 0xCABAC28A 0x53B39330 0x24B4A3A6
                    0xBAD03605 0xCDD70693 0x54DE5729 0x23D967BF
                    0xB3667A2E 0xC4614AB8 0x5D681B02 0x2A6F2B94
                    0xB40BBE37 0xC30C8EA1 0x5A05DF1B 0x2D02EF8D
}

proc crc32 {instr} {
    global CRCTABLE

    set crc_value 0xFFFFFFFF
    foreach c [split $instr {}] {
        set crc_value [expr {[lindex $CRCTABLE [expr {($crc_value ^ [scan $c %c])&0xff}]]^(($crc_value>>8)&0xffffff)}]
    }
    return [expr {$crc_value ^ 0xFFFFFFFF}]
}

proc crc32_binary {file} {
    set f [open $file r]
    fconfigure $f -translation binary
    set data [read $f]
    close $f
    return [crc32 $data]
}


#
# Flash loader binary
#
array set rtl872x_flash_loader {
    0 0x00082021 1 0x00082021 2 0x00082021 3 0x00082021 4 0x00082021 5 0x00000000
    6 0x00000000 7 0x00000000 8 0x46DEB5F0 9 0x46574645 10 0xF245464E 11 0xB5E05375
    12 0x0300F2C0 13 0x2003B087 14 0xF6424798 15 0xF2420440 16 0xF2C020C4 17 0xF2400408
    18 0xF2C041B0 19 0xF77E0008 20 0x0021FA11 21 0x20F4F242 22 0xF2C03134 23 0xF77E0008
    24 0x0021FA09 25 0x3024F242 26 0xF2C03130 27 0xF77E0008 28 0x0021FA01 29 0x3054F242
    30 0xF2C03140 31 0xF77E0008 32 0xF242F9F9 33 0xF24231C0 34 0xF2C03084 35 0xF2C00108
    36 0xF77E0008 37 0xF240F9EF 38 0xF6C42210 39 0x68130200 40 0xF2404987 41 0x400B15F8
    42 0x2100F240 43 0x6013430B 44 0x4351F242 45 0x0508F2C0 46 0xF2C00028 47 0x47980300
    48 0x43E1F242 49 0xF2C02000 50 0x47980300 51 0x5CE8234D 52 0x531DF641 53 0xF2C0AA05
    54 0x21030300 55 0xAB044798 56 0x2B20791B 57 0x22C7D102 58 0x54EA3332 59 0x33C0F242
    60 0x0308F2C0 61 0xF2469301 62 0xF2C063AD 63 0x93000300 64 0x5BF5F641 65 0x5391F246
    66 0x0A49F242 67 0x0300F2C0 68 0x0B00F2C0 69 0xF2429302 70 0xF2C01747 71 0x465B0A00
    72 0xF2C04656 73 0x93030700 74 0x6B632500 75 0xD0FC2B00 76 0x30C0F242 77 0xF2409B00
    78 0x210042B0 79 0x0008F2C0 80 0x47986365 81 0x2B126C23 82 0xD937D044 83 0xD1002B14
    84 0xD35FE082 85 0xD1E82B15 86 0x6C632280 87 0x6CA16CE0 88 0x189A0512 89 0x29004680
    90 0xE0A6D100 91 0x2180185B 92 0x46890509 93 0x23014499 94 0x200146A4 95 0x464C0015
    96 0x782A425B 97 0x22084053 98 0x40030859 99 0xF248B11B 100 0xF6CE3320 101 0x3A0153B8
    102 0x2A00404B 103 0x3501D1F4 104 0xD1EE42AC 105 0x43DB4664 106 0x1AD34642 107 0x41931E5A
    108 0xB2DB9A01 109 0x23017013 110 0xE7B56323 111 0xD06D2B10 112 0xD1B22B11 113 0x20002100
    114 0x9B0147B8 115 0x2301701D 116 0xE7A96323 117 0x25006CA3 118 0x6C634699 119 0x4698031B
    120 0xB18B464B 121 0x00330022 122 0x464E002C 123 0x46994645 124 0x00294690 125 0x47B82002
    126 0x0C00F241 127 0x44653401 128 0xD1F642A6 129 0x464E4644 130 0x9A012300 131 0x33017013
    132 0xE7896323 133 0x25006C63 134 0x6CA3469A 135 0xD0F32B00 136 0x001C46A1 137 0x46560033
    138 0x46B8469A 139 0xF2429F03 140 0xF2C032C0 141 0x46940208 142 0x44631D2B 143 0x2A101B62
    144 0x2210D900 145 0x20001971 146 0x47B83510 147 0xD8EE42AC 148 0x464C4647 149 0xE7D74656
    150 0x0A40F240 151 0x25006C63 152 0x6CA3469B 153 0x469944A2 154 0xD0CD2B00 155 0x46B00023
    156 0x465E464C 157 0x465746B9 158 0x002A469A 159 0x18BA3210 160 0x29081B61 161 0x2108D900
    162 0xB2C91970 163 0x47C03508 164 0xD8F242AC 165 0x4654464F 166 0xE7B54646 167 0x791BAB04
    168 0x70D3AA04 169 0x22019B01 170 0x1D18701D 171 0x1CD9AB04 172 0x47989B02 173 0x63232301
    174 0x2300E736 175 0x46C0E774 176 0xFFFFF9FF 177 0x20746573 178 0x5F47534D 179 0x5F465542
    180 0x455A4953 181 0x20202020 182 0x20202020 183 0x20202020 184 0x20202020 185 0x20202020
    186 0x20202020 187 0x0A0D6425 188 0x00000000 189 0x20746573 190 0x6D5F6370 191 0x615F6773
    192 0x6C696176 193 0x6D656D5F 194 0x6464615F 195 0x20202072 196 0x20202020 197 0x20202020
    198 0x20202020 199 0x70383025 200 0x00000A0D 201 0x20746573 202 0x5F75636D 203 0x5F67736D
    204 0x69617661 205 0x656D5F6C 206 0x64615F6D 207 0x20207264 208 0x20202020 209 0x20202020
    210 0x20202020 211 0x70383025 212 0x00000A0D 213 0x20746573 214 0x6D5F6370 215 0x645F6773
    216 0x5F617461 217 0x5F6D656D 218 0x72646461 219 0x20202020 220 0x20202020 221 0x20202020
    222 0x20202020 223 0x70383025 224 0x00000A0D 225 0x20746573 226 0x5F75636D 227 0x5F67736D
    228 0x61746164 229 0x6D656D5F 230 0x6464615F 231 0x20202072 232 0x20202020 233 0x20202020
    234 0x20202020 235 0x70383025 236 0x00000A0D
}
