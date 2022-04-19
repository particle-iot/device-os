function die
{
   exit 1
}

bats build.bats || die
bats boron.bats || die
bats libs.bats || die
