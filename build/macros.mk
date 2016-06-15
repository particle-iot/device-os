include $(COMMON_BUILD)/os.mk
include $(COMMON_BUILD)/verbose.mk

# recursively finds files matching the given pattern
# $1 is the directory to search for files (should end with a slash)
# $2 is the wildcard specifying the files to find
rwildcard = $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

appendir = $(and $1,/$1)
# remove up directories so that we don't jump up levels
sanitize = $(pathsubst ..,.,$1)

ifneq (OSX,$(MAKE_OS))
filesize=`stat --print %s $1`
else
filesize=`stat -f%z $1`
endif

# fetches the byte at a given offset in a file
# $1 the file to fetch the byte from
# $2 the offset in the file of the byte to fetch
#
filebyte=`dd if=$1 skip=$2 bs=1 count=1 | xxd -p`

test=$(strip $(shell if test $1 $2 $3; then echo 1; fi))
_assert_equal = $(if $(call test,$2,-ne,$3),$(error "expected $1 to be $2 but was $3))

# asserts the given file is the given size
# $1 the file to test
# $2 the expected size of the file in decimal
assert_filesize = $(call _assert_equal,"file $1",$2,$(shell echo $(call filesize,$1)))

assert_filebyte = $(call _assert_equal,"file $1 offset $2",$3,$(shell echo $(call filebyte,$1,$2)))

# Recursive wildcard function - finds matching files in a directory tree
target_files = $(patsubst $(SOURCE_PATH)/%,%,$(call rwildcard,$(SOURCE_PATH)/$1,$2))
here_files = $(call wildcard,$(SOURCE_PATH)/$1$2)

remove_slash = $(patsubst %/,%,$1)
add_slash = $(patsubst %, %/,$(call remove_slash,$1))

# enumerates the files across a number of directories
# $1 the list of directories to search. Each directory is searched recursively
# $2 the subdirectory relative to each directory to search - must end with a slash
# $3 the wildcard to search within each directory
# Each file is returned relative to the directory it was (recursively) searched in, prepended
# with the name of the search directory.
# E.g.
# /files/libs/mylib/abc.cpp ->  libs/mylib/abc.cpp when the search is in directory /files/libs with wildcard *.cpp
target_files_dirs = $(foreach d,$(call remove_slash,$1),$(patsubst $d/%,$(notdir $d)/%,$(call rwildcard,$d/$2,$3)))

check_modular = $(if $(PLATFORM_DYNALIB_MODULES),,$(error "Platform '$(PLATFORM)' does not support dynamic modules"))
