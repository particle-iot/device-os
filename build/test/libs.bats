load build


@test "can build a project with external libraries" {
    cd modules/photon/user-part
    outdir=../build/test/files/applibs/app/target
    rm -rf $outdir
    run make $make_args PLATFORM=photon APPDIR=../build/test/files/applibs/app "APPLIBSV1=../build/test/files/applibs/lib1/ ../build/test/files/applibs/lib2/"
    [ $status -eq 0 ]
    [ -s $outdir/app.bin ]
    file_size_range $outdir/app.bin 2 5 K 
}


@test "can build a project with external libraries in a directory containing spaces" {
    skip # the make system does not support directories with spaces
    cd modules/photon/user-part
    outdir=../build/test/files/applibs\ spaces/app1/target
    rm -rf "$outdir"
    run make $make_args PLATFORM=photon "APPDIR=../build/test/files/applibs spaces/app1" "\"APPLIBSV1=../build/test/files/applibs spaces/lib1/\" \"../build/test/files/applibs spaces/lib2/\""
    [ $status -eq 0 ]
    [ -s "$outdir/app1.bin" ]
    file_size_range "$outdir/app1.bin" 2 5 K 
}


@test "can build a v1 project with external v2 libraries" {
    cd modules/photon/user-part
    outdir=../build/test/files/applibs_v2/app/target
    rm -rf $outdir
    run make $make_args PLATFORM=photon APPDIR=../build/test/files/applibs_v2/app "APPLIBSV2=../build/test/files/applibs_v2/lib1/ ../build/test/files/applibs_v2/lib2"
    [ $status -eq 0 ]
    [ -s $outdir/app.bin ]
    file_size_range $outdir/app.bin 2 5 K 
}


@test "can build a v2 project with explicitly set vendored v2 libraries" {
    cd modules/photon/user-part
    outdir=../build/test/files/applibs_v2_vendored/app_explicit/src/target
    rm -rf $outdir
    run make $make_args PLATFORM=photon APPDIR=../build/test/files/applibs_v2_vendored/app_explicit/src "APPLIBSV2=../build/test/files/applibs_v2_vendored/app/lib/lib1/ ../build/test/files/applibs_v2_vendored/app/lib/lib2"
    [ $status -eq 0 ]
    [ -s $outdir/src.bin ]
    file_size_range $outdir/src.bin 2 5 K 
}


@test "can build a v2 project with implicity set vendored v2 libraries" {
    cd modules/photon/user-part
    outdir=../build/test/files/applibs_v2_vendored/app/target
    rm -rf $outdir
    run make $make_args PLATFORM=photon APPDIR=../build/test/files/applibs_v2_vendored/app
    [ $status -eq 0 ]
    [ -s $outdir/app.bin ]
    file_size_range $outdir/app.bin 2 5 K 
}
