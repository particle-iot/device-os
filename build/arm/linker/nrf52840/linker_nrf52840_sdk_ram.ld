SECTIONS
{
    . = ALIGN(4);
    .svc_data :
    {
        PROVIDE(__start_svc_data = .);
        KEEP(*(.svc_data))
        PROVIDE(__stop_svc_data = .);
    } > SRAM
    
    .fs_data :
    {
        PROVIDE(__start_fs_data = .);
        KEEP(*(.fs_data))
        PROVIDE(__stop_fs_data = .);
    } > SRAM
    
    .log_dynamic_data :
    {
        PROVIDE(__start_log_dynamic_data = .);
        KEEP(*(SORT(.log_dynamic_data*)))
        PROVIDE(__stop_log_dynamic_data = .);
    } > SRAM
    
    .cli_sorted_cmd_ptrs :
    {
        PROVIDE(__start_cli_sorted_cmd_ptrs = .);
        KEEP(*(.cli_sorted_cmd_ptrs))
        PROVIDE(__stop_cli_sorted_cmd_ptrs = .);
    } > SRAM
} INSERT AFTER .data;
