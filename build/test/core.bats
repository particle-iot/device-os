load build

@test "core build wiring/api test" {
    cd main
    run make PLATFORM=core TEST=wiring/api
    outdir=../build/target/main/platform-0-lto
    [ -s $outdir/api.bin ]
}

@test "core build tinker app" {
    cd main
    run make PLATFORM=core APP=tinker
    outdir=../build/target/main/platform-0-lto
    [ -s $outdir/tinker.bin ]
}

@test "clean core main build" {
    run force_clean
    cd main
    run make
    outdir="../build/target/main/platform-0-lto"
    bldir="../build/target/bootloader/platform-0-lto"
    [ ! -f $bldir/bootloader.bin ]
    [ -d $outdir ]
    [ -s $outdir/main.bin ]
    [ -s $outdir/main.elf ]
    file_size_range $outdir/main.bin 70 85 K
}

@test "repeat core main build silent outputs size" {
    cd main
    run make PLATFORM=core -s
    outdir="../build/target/main/platform-0-lto"
    assert_equal "text data bss dec hex filename" "$(trim ${lines[0]})"
    assert_equal "2" "${#lines[@]}"
}

