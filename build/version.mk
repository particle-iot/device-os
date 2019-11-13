VERSION_STRING = 1.4.2

# PRODUCT_FIRMWARE_VERSION reported by default
# FIXME: Unclear if this is used, PRODUCT_FIRMWARE_VERSION defaults to 65535 every release
VERSION = 1404

CFLAGS += -DSYSTEM_VERSION_STRING=$(VERSION_STRING)

ifneq "$(wildcard $(APPDIR)/.git )" "" #check if local repo exist
#Create variable GIT_MSG with:
#1) git sha and additionally dirty flag
GIT_MSG := sha:$(shell git --git-dir=${APPDIR}/.git --work-tree=${APPDIR} --no-pager describe --tags --always --dirty)
#2) last commit msg
GIT_MSG += msg:$(shell git --git-dir=${APPDIR}/.git --work-tree=${APPDIR} --no-pager log -1 --pretty=%B)
else
GIT_MSG := Can't find git repo
endif
#add user define macro (-D) for gcc preprocessor
CFLAGS += -DGIT_MSG=\"$(strip "$(GIT_MSG)")\"
