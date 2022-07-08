    .note.gnu.build-id :
    {
        __start_gnu_build_id_start = .;
        link_gnu_build_id_location_start = .;
        KEEP(*(.note.gnu.build-id))
        link_gnu_build_id_location_end = .;
    } >APP_FLASH  AT> APP_FLASH
