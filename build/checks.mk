
git_branch := $(shell git rev-parse --abbrev-ref HEAD)
ifeq ("$(git_branch)","develop")
    ifndef PARTICLE_DEVELOP
       $(error Please note the develop branch contains untested, unreleased code. \
        We recommend using the 'latest' branch which contains the latest released firmware code. \
        To build the develop branch, please see the the build documentation at \
        https://github.com/spark/firmware/blob/develop/docs/build.md#building-the-develop-branch)
    endif
endif
