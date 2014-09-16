This firmware repo (formerly, core-firmware) is a refactor of these previous repos:

- bootloader
- core-common-lib
- core-communications-lib
- core-firmware

The merge was executed into the core-firmware repo, in the feature/hal branch. The initial state of the other repos was also taken from the feature/hal branch.

Initially all repos were merged into a directory based on their name

- bootloader --> bootloader/
- core-common-lib --> common/
- core-communications-lib -->  communications/

At present, there is no top-level build/ directory. To build firmware, cd to 
`firmware/build` and run make as usual. Similarly for the bootloader.

When merging unmerged branches from the external repos, first add the external repos as
remotes to the firmware repo. Then you can switch to the local feature branch, and merge
the changes, taking into account the new directory structure, with -Xsubtree, e.g.

```
git checkout -b feature-in-progress
git merge -s recursive -Xsubtree=repodir repodir/feature-in-progress
```
[source](http://saintgimp.org/2013/01/22/merging-two-git-repositories-into-one-repository-without-losing-file-history/)


Todo:
- top level build that makes everything
- reinstate travis-ci build
- factor common elements of bootloader and firmware (e.g. startup scripts, linker files.)
- factor firmware into wiring and main (for want of a better name)
- add include.mk to each project that adds it's publicly visible includes to INCLUDE_DIRS