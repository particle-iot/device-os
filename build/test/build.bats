
load assert

function setup()
{
    # get in the project root
    pushd ../..
}

function force_clean() {
   rm -rf build/target
}


function file_size_range() {
file=$1
scale=${4:=1}
if [ "$scale" == "K" ]; then
scale=1024
fi
minimumsize=$(($2*$scale))
maximumsize=$(($3*$scale))

actualsize=$(wc -c <"$file")
if [ "$actualsize" -le "$minimumsize" ]; then
    echo "file '$file' size is $actualsize which is less than $minimumsize"
    return -1
else
    if [ "$actualsize" -ge "$maximumsize" ]; then
        echo "file '$file' size is $actualsize which is greater than $maximumsize"
        return 1
    fi
fi
return 0
}

@test "make is version 3.81" {
   run make --version
   [ "${lines[0]}" == 'GNU Make 3.81' ]
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

@test "clean core main build" {
    run force_clean
    cd main
    run make
    outdir="../build/target/main/platform-0-lto"
    bldir="../build/target/bootloader/platform-0-lto"
    assert_equal "$(pwd)" "/Users/mat1/dev/brewpi/spark-firmware/main"
    [ ! -f $bldir/bootloader.bin ]
    [ -d $outdir ]
    [ -s $outdir/main.bin ]
    [ -s $outdir/main.elf ]
    file_size_range $outdir/main.bin 70 85 K
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


@test "clean photon mono main build" {
    run force_clean
    cd main
    run make PLATFORM=photon MODULAR=n
    outdir=../build/target/main/platform-6-lto
    [ -s $outdir/main.bin ]
    [ -s $outdir/main.elf ]
    file_size_range $outdir/main.bin 340 420 K
}

