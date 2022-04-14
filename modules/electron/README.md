## A note on module numbers and versions.

TL;DR: the directory names (part1, part2, part3) do not correspond with the internal symbol names and module identifier. 

system-part1: system module 3
system-part2: system-module 1
system-part3: system module 2

Consequently, you'll see that the symbols used do not reflect the diretory name.  for example system-part1 dependencies have to be defined using the symbol `SYSTEM_PART3_MODULE_DEPENDENCY` since system-part1 contains module 3.

Yes, it's nuts and it's done for hysterical reasons. 

## Some history and hysterics...

The initial releases of the Electron had system-part1 and system-part2 modules, in the same fashion as the P1/Photon. Once we had filled up space, some functions were branched out into a third module, system-part3. In order for things to work, this module was made a dependent of system-part1.  So the flash order would be

- system-part3
- system-part1  (depends on system part3)
- system-part2  (depends on system part1)

However, from the UX perspective this seemed counter-intuitive. It's  more intuitive to expect to flash these modules in numerical order.  

To make that possible, we merely renamed the directories and the `make` module name, so that the filenames produced follow the order expected.  This makes it easy for users.  But ofr maintainers, it's a bit of a headache since the internal module part number remains the same as it was, and does not reflect the module filename. 

We need to be mindful of this, since internally within the build scripts the old names are still used. This is why system-part1 dependencies are defined using the symbol `SYSTEM_PART3_MODULE_DEPENDENCY`