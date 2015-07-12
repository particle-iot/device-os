
load build

@test "make is version 3.81" {
   run make --version
   [ "${lines[0]}" == 'GNU Make 3.81' ]
}
