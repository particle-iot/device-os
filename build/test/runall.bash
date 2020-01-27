
function die
{
   exit 1
}

bats build.bats || die
bats photon.bats || die
bats libs.bats || die
