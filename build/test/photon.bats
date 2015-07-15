load build


@test "photon wiring/api" {
# TODO - I think the build should output to main rather than user-part.
# And to a test/wiring/api subdirectory, just as it would be when building from main
    cd main
    run make PLATFORM=photon TEST=wiring/api
    outdir=../build/target/user-part/platform-6-m-lto
    [ -s $outdir/api.bin ]
}

@test "clean photon modular main build" {
    run force_clean
    cd main
    run make PLATFORM=photon
    outdir=../build/target/user-part/platform-6-m-lto
    [ -s $outdir/user-part.bin ]
    [ -s $outdir/user-part.elf ]
    file_size_range $outdir/user-part.bin 2 8 K
}

@test "photon modular main build to relative external folder" {
    cd build
    rm -rf target/xyz
    bad_target=../modules/photon/build/target/xyz/abc.bin
    [[ ! -f $bad_target ]] || rm $bad_target
    run make -C ../main v=1 PLATFORM=photon TARGET_DIR=../build/target/xyz TARGET_FILE=abc
    outdir=target/xyz
    [ ! -f ../modules/photon/build/target/xyz/abc.bin ]
    [ -d $outdir ]
    [ -s $outdir/abc.bin ]
    [ -s $outdir/abc.elf ]
    file_size_range $outdir/abc.bin 2 8 K
}

@test "clean photon mono main build" {
    run force_clean
    cd main
    run make PLATFORM=photon MODULAR=n
    outdir=../build/target/main/platform-6-lto
    [ -s $outdir/main.bin ]
    [ -s $outdir/main.elf ]
    file_size_range $outdir/main.bin 340 420 K
}


