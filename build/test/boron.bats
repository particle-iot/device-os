load build

@test "clean boron modular main build" {
    run force_clean
    cd main
    run make PLATFORM=boron $make_args
    outdir=../build/target/user-part/platform-13-m
    [ -s $outdir/user-part.bin ]
    [ -s $outdir/user-part.elf ]
    file_size_range $outdir/user-part.bin 2 8 K
}

@test "boron wiring/api" {
# TODO - I think the build should output to main rather than user-part.
# And to a test/wiring/api subdirectory, just as it would be when building from main
    cd main
    run make PLATFORM=boron TEST=wiring/api $make_args
    outdir=../build/target/user-part/platform-13-m
    [ -s $outdir/api.bin ]
}

@test "boron modular main build to relative external folder" {
    cd build
    rm -rf target/xyz
    bad_target=../modules/boron/build/target/xyz/abc.bin
    [[ ! -f $bad_target ]] || rm $bad_target
    run make -C ../main v=1 PLATFORM=boron TARGET_DIR=../build/target/xyz TARGET_FILE=abc $(make_args)
    outdir=target/xyz
    [ ! -f ../modules/boron/build/target/xyz/abc.bin ]
    [ -d $outdir ]
    [ -s $outdir/abc.bin ]
    [ -s $outdir/abc.elf ]
    file_size_range $outdir/abc.bin 2 8 K
}

@test "clean boron mono main build" {
    run force_clean
    cd main
    run make PLATFORM=boron MODULAR=n $make_args
    outdir=../build/target/main/platform-13
    [ -s $outdir/main.bin ]
    [ -s $outdir/main.elf ]
    file_size_range $outdir/main.bin 450 550 K
}
