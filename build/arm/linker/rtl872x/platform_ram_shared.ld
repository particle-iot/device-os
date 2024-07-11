/* FIXME: duplicates platform_ram_size in platform_ram.ld */
platform_km4_ram_size  = 476K;
platform_km4_ram_start = 0x10005000;
platform_km4_ram_end   = platform_km4_ram_start + platform_km4_ram_size;

platform_km4_ram_reserved = 4K;

platform_km0_part1_wifi_fw_ram_start = platform_km4_ram_start + platform_km4_ram_reserved;
platform_km0_part1_wifi_fw_ram_size = 80K;
platform_km0_part1_wifi_fw_ram_end = platform_km0_part1_wifi_fw_ram_start + platform_km0_part1_wifi_fw_ram_size;
