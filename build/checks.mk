
git_branch := $(shell git rev-parse --abbrev-ref HEAD)
$(info "branch $(git_branch)")
ifeq ("$(git_branch)","develop")
    ifndef PARTICLE_DEVELOP
       $(error Please note the develop branch contains untested, unreleased code. \
        We recommend using the 'photon' branch which contains the latest released firmware code. \
        To build the develop branch, please see the build documentation.)
    endif
endif