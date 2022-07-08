    .mem_section_dummy_rom :
    {
    }
    
    .sdh_soc_observers :
    {
        PROVIDE(__start_sdh_soc_observers = .);
        KEEP(*(SORT(.sdh_soc_observers*)))
        PROVIDE(__stop_sdh_soc_observers = .);
    } > APP_FLASH AT> APP_FLASH
    
    .sdh_ble_observers :
    {
        PROVIDE(__start_sdh_ble_observers = .);
        KEEP(*(SORT(.sdh_ble_observers*)))
        PROVIDE(__stop_sdh_ble_observers = .);
    } > APP_FLASH AT> APP_FLASH
    
    .pwr_mgmt_data :
    {
        PROVIDE(__start_pwr_mgmt_data = .);
        KEEP(*(SORT(.pwr_mgmt_data*)))
        PROVIDE(__stop_pwr_mgmt_data = .);
    } > APP_FLASH AT> APP_FLASH
    
    .log_const_data :
    {
        PROVIDE(__start_log_const_data = .);
        KEEP(*(SORT(.log_const_data*)))
        PROVIDE(__stop_log_const_data = .);
    } > APP_FLASH AT> APP_FLASH
    
    .nrf_balloc :
    {
        PROVIDE(__start_nrf_balloc = .);
        KEEP(*(.nrf_balloc))
        PROVIDE(__stop_nrf_balloc = .);
    } > APP_FLASH AT> APP_FLASH
    
    .sdh_state_observers :
    {
        PROVIDE(__start_sdh_state_observers = .);
        KEEP(*(SORT(.sdh_state_observers*)))
        PROVIDE(__stop_sdh_state_observers = .);
    } > APP_FLASH AT> APP_FLASH
    
    .sdh_stack_observers :
    {
        PROVIDE(__start_sdh_stack_observers = .);
        KEEP(*(SORT(.sdh_stack_observers*)))
        PROVIDE(__stop_sdh_stack_observers = .);
    } > APP_FLASH AT> APP_FLASH
    
    .sdh_req_observers :
    {
        PROVIDE(__start_sdh_req_observers = .);
        KEEP(*(SORT(.sdh_req_observers*)))
        PROVIDE(__stop_sdh_req_observers = .);
    } > APP_FLASH AT> APP_FLASH
    
    .nrf_queue :
    {
        PROVIDE(__start_nrf_queue = .);
        KEEP(*(.nrf_queue))
        PROVIDE(__stop_nrf_queue = .);
    } > APP_FLASH AT> APP_FLASH
    
    .cli_command :
    {
        PROVIDE(__start_cli_command = .);
        KEEP(*(.cli_command))
        PROVIDE(__stop_cli_command = .);
    } > APP_FLASH AT> APP_FLASH
    
    .crypto_data :
    {
        PROVIDE(__start_crypto_data = .);
        KEEP(*(SORT(.crypto_data*)))
        PROVIDE(__stop_crypto_data = .);
    } > APP_FLASH AT> APP_FLASH
