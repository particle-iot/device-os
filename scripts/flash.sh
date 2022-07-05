#!/bin/bash
OPENOCD=$(which openocd)
$OPENOCD -f interface/cmsis-dap.cfg -c "transport select swd" \
    -f $1 \
    -c init -c "reset halt" \
    -c "rtl872x_flash_write_bin $2  $3" \
    -c "rtl872x_wdg_reset" \
    -c shutdown
