# Contributions

All contributors must first sign the [Particle Individual Contributor License Agreement (CLA)](https://docs.google.com/a/spark.io/forms/d/1_2P-vRKGUFg5bmpcKLHO_qNZWGi5HKYnfrrkd-sbZoA/viewform), which is based on the Google CLA, and provides the Particle team a license to re-distribute your contributions.

Whenever possible, please follow these guidelines for contributions:

- Keep each pull request small and focused on a single feature or bug fix.
- Familiarize yourself with the code base, and follow the formatting principles adhered to in the surrounding code.
- Wherever possible, provide unit tests for your contributions.

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

- communication/lib/libcoap

(you can find an up-to-date list by running git log | grep git-subtree-dir | awk '{ print $2 }')

When making commits, do not mix commits that span subtrees. E.g. a commit that posts
changes to files in communication/lib/libcoap and communication/src should be avoided.
No real harm is done, but the commit messages can be confusing since those files outside the subtree
will be removed when the subtree is pushed to the upstream repo.

To avoid any issues with subtres, it's simplest to make all changes to the upstream repo via
PRs from a separate working copy, just as you would with any regular repo. Then pull these changes
into the subtree in this repo.


