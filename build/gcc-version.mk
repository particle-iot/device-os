version_to_number=$(shell v=$1; v=($${v//./ }); echo $$((v[0] * 10000 + v[1] * 100 + v[2])))
get_major_version=$(shell v=$1; v=($${v//./ }); echo $${v[0]})
gcc_version_str:=$(shell $(CC) -dumpfullversion)
gcc_version:=$(call version_to_number,$(arm_gcc_version_str))
