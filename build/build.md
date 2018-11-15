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

## Module Variables

### Target file variables that don't need to be set

- `TARGET`: the target file(s) being constructed by a given module. Default is `$(TARGET_BASE).$(TARGET_TYPE)`. This is set by 
   the modules that build the final binary to the list of pseudo-targets to be built, (e.g. elf lst size). It is incorporated into the `all` goal so that the target(s) are built. 
- `TARGET_BASE`: the target file without the extension
- `TARGET_NAME`: the final target filename (minus the extension). Default is `$(TARGET_FILE_PREFIX)$(TARGET_FILE_NAME)`
- `TARGET_FILE_PREFIX`: Empty by default. Used to add a prefix to the target. This is used to add a `lib` prefix to library output files. 
- `TARGET_PATH`: the output directory for the target file. Default is `(BUILD_PATH)/$(call sanitize,$(TARGET_DIR_NAME))` 
- `BUILD_PATH`: the build path for this module
- `MODULAR_EXT`: adds an extension to the platform variant in the output path so that monolithic and modular builds go to separate output directories.
- `LTO_EXT`: adds an extension to the platform variant in the output path so that LTO and regular builds go to separate output directories.
`BUILD_PATH_EXT`: the directory inside the `build/target/<module>` folder that contains subdirectories for this given build platform (and variant), e.g. `platform-14-m-lto` for platform 14, modular build with LTO.


## Recursion and Module Dependencies

There are two types of dependency:

- *file system dependencies* are dependencies (other modules) that are needed to build the current module. It is typically the public interface of the dependency module. The `import.mk` of these dependencies are loaded and are used to set flags, set include directories. These modules are not built since it is assumed everything needed is already present in the filesystem.

- *built dependencies* are modules that are recursively invoked with `make` and the same targets. This is used to build artefacts that the calling module needs, such as a library.

The `DEPENDENCIES` variable lists the file system dependencies, while the `MAKE_DEPENDENCIES` file lists the built dependencies. It's quite typical that all `MAKE_DEPENDENCIES` also feature as `DEPENDENCIES` since the public interface (such as headers) is needed to consume the built artefacts of the module.

The goals of the module are not passed down to submodules. This is a bit of a hack, most likely
to avoid top-level goals such as `porgram-dfu` being passed down to submodules where they make no sense. The hack works since typically submodules have only one primary goal which is invoked as if `all` were invoked.

However, this is unlikely to remain the case, so goals that make no sense to submodules should be filtered out rather than remove all goals. 