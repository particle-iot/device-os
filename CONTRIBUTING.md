# Contributions

All contributors must first sign the [Particle Individual Contributor License Agreement (CLA)](https://docs.google.com/a/spark.io/forms/d/1_2P-vRKGUFg5bmpcKLHO_qNZWGi5HKYnfrrkd-sbZoA/viewform), which is based on the Google CLA, and provides the Particle team a license to re-distribute your contributions.

Whenever possible, please follow these guidelines for contributions:

- Keep each pull request small and focused on a single feature or bug fix.
- Familiarize yourself with the code base, and follow the formatting principles adhered to in the surrounding code.
- Wherever possible, provide unit tests for your contributions.
- If the changes have an impact application developers, then those changes should be described in the documentation. 


# Tests and Documentation

Any changes that affect firmware application developers, such as a new class or API, or change in existing behavior should have an accompanying PR to the `spark/docs` repo describing the changes. This ensures the documentation is kept up to date with changes to the firmware.

New firmware APIs should have an API test. These are found in the `user/tests/wiring/api` folder. see [Building Platform Tests](https://github.com/spark/firmware/blob/develop/user/tests/readme.md#building-platform-tests).

New functionality should also have a corresponding unit test. These are found in `user/tests/wiring/no_fixture` when they do not require any additional test fixture beyond the device itself. Tests requiring a hardware fixture are found in the varoius subdirecotires. If the hardware fixture you need is already present, then feel free to add more tests to that. When the hardware fixture is not present, simply create a new directory, following the pattern of the existing tests (and by copying application.cpp from an existing test.)



# Changes to Functions, Structures and Dynalib Tables

With dynamic linking, APIs must remain backwards compatible at the ABI level, since newer code may be called by older clients. 

Here are some guidelines to follow to ensure compatibility for structs and functions used in a dynamic interface:

- Do not add or remove arguments to the function signature. If the function has
a final reserved `void*` this may be changed to name a struct that is optionally passed to the function. The struct should have it's size as the first member. If the function already has such a struct, no changes other than renaming fields (which doesn't affect the binary) or adding fields at the end of the struct are allowed.

- Tables of dyanmic entrypoints should not be changed, in particular, the order
of the `DYNALIB_FN` entries must remain unchanged. New entries can be added at the end. Ensure that the function can evolve by including a `void*` reserved parameter as the last parameter. 




# Subtrees

The repository imports content from other repos via git subtrees. These are the current
subtrees:

- communication/lib/mbedtls

(you can find an up-to-date list by running git log | grep git-subtree-dir | awk '{ print $2 }')

When making commits, do not mix commits that span subtrees. E.g. a commit that posts
changes to files in communication/lib/libcoap and communication/src should be avoided.
No real harm is done, but the commit messages can be confusing since those files outside the subtree
will be removed when the subtree is pushed to the upstream repo.

To avoid any issues with subtres, it's simplest to make all changes to the upstream repo via
PRs from a separate working copy, just as you would with any regular repo. Then pull these changes
into the subtree in this repo.


