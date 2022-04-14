Artifact Versioning and Tagging (1.0.0)
=======================================

Artifact Name Composition
-------------------------

### Filenames

|artifact name|@|semantic version|.|file extension|
|:-:|:-:|:-:|:-:|:-:|
|boron-bootloader|@|1.0.1-rc.1|.|bin|

**E.g.** `boron-bootloader@1.0.1-rc.1.bin`

#### Filename Composition

- Artifact Name
  - Platform Name (`boron`)
  - Module Name (`bootloader`)
- Semantic Version
  - Major Version (`1`)
  - Minor Version (`0`)
  - Patch Version (`1`)
  - [_OPTIONAL_] Prerelease Tag (`rc.1`)
  - [_OPTIONAL_] Build Metadata 1,2,...,n (`lto`, `debug`, `jtag`)
- File Extension (`bin`)

**Rules:**

- Boolean build parameters are added to metadata when `true` and absent when `false`
- Enumerated build parameter "values" are added to metadata

**Seperators:**

- `-` (_hyphen_) - Seperates platform and modules names
- `@` (_at symbol_) - Seperates the artifact name from the semantic version
- `.` (_period_) - Precedes the file extension

> _**NOTE:** Seperators belonging to the [semantic versioning specification](https://semver.org/) will be nested within the binary name. Those seperators are NOT listed here._

Folder Hierarchy
----------------

|evaluated semantic version|/|platform|/|binary descriptor|
|:-:|:-:|:-:|:-:|:-:|
|1.0.1|/|boron|/|debug|

**E.g.** `1.0.1/boron/debug`

### Hierarchical Composition

- Evaluated Semantic Version*
  - Major Version (`1`)
  - Minor Version (`0`)
  - Patch Version (`1`)
  - [_OPTIONAL_] Pre-release Tag (_none_)
- Platform Name (`boron`)
- Binary Descriptor (`debug`)

Zip Artifacts
-------------

|company|_|product|@|evaluated semantic version|+|platform|.|content|.|file_extension|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|particle|_|device-os|@|1.0.1|+|boron|.|binaries|.|zip|

**E.g.** `particle_device-os@1.0.1+boron.binaries.zip`

### Zip Filename Composition

- Artifact Name
  - Company Name (`particle`)
  - Product Name (`device-os`)
- Evaluated Semantic Version*
  - Major Version (`1`)
  - Minor Version (`0`)
  - Patch Version (`1`)
  - [_OPTIONAL_] Prerelease Tag (_none_)
- Filter
  - [_OPTIONAL_] Platform Name (`boron`)
  - [_OPTIONAL_] Content Type (`binaries`)
- File Extension (`bin`)

**Rules:**

- Multi-word names should be express with as hyphenated values (i.e. `device-os`)

**Seperators:**

- `_` (_underscore_) - Seperates company and product names
- `@` (_at symbol_) - Seperates the artifact name from the evaluated semantic version
- `+` (_plus sign_) - Seperates the evaluated semantic version from the filters
- `.` (_period_) - Seperates each filter and precedes the file extension

> _**NOTE:** Seperators belonging to the [semantic versioning specification](https://semver.org/) will be nested within the binary name. Those seperators are NOT listed here._

Definitions
-----------

- **evaluated semantic version** - The version information considered when deciding precedence.

Reference Implementation
------------------------

[make_release.sh](../build/make_release.sh) - The script used to derive binaries for release on GitHub.

> _**NOTE:** For greater detail, see help section via `make_release.sh --help`._

Addendum
--------

- Source of Evaluated Semantic Version:
  - [release.sh](../build/release.sh) - The original build script.
