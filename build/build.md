# Make system outline notes

Each module is built using it's makefile in the root. That makefile defines
 - the module name
 - the target path and type (e.g. .a for libs, .elf/.exe for main)
 - module dependencies (e.g. headers, build not required)
 - module artefact dependencies (requires the module to be built, e.g. library)

The makefile also pulls in `arm-tlm.mk` which fills out the rest of build
infrastructure for a top level module.

The product-id matrix is defined in `product-id.mk` and maps the supplied product
ID to orthogonal aspects of the product.

The module's primary build script resides in `build.mk` in the module folder.
This typically contains recipes for building the module's artefacts.

The `import.mk` makefile is used both when a module is built (using the makefile in
the module directory) and when imported as a dependency of another module.
It defines the files that a module exports as publicly visible,
such as the final produced artefacts and include files. This ensures these details
are specified only once - in the module - rather than being duplicated
again by the module's consumers.

Some modules produce multiple distinct "flavors" of their artefacts, by building
to a distinct target directory. Typical distinguishing features of different
flavors are:

- product ID
- architecture (arm / gcc)
- build / debug (not yet implemented.)
- test driver / test subject

Having separate flavors (to distinct build targets) ensures that they are
consistently rebuilt when needed which would not be the case otherwise.



