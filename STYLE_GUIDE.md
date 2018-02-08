This is the beginning of the style guide for the Particle Firmware repository.
It's presently just an outline, and will be fleshed out over time.

Note that it will be easy to find violations of these guidelines in the existing codebase.
Over time, these will be rectified as automated checks for style consistency are introduced.


## Formatting

- indentation at 4 spaces
- braces on the next line for function bodies
- single statement bodies (`if`/`for` etc..) should always have braces, even though they are notstrictly required. This is to reduce errors due to missing braces for multi-line bodies the code is extended. 
- functions/methods should generally be smaller than 40 lines and avoid nesting deeper than 6 levels

## doc-comments
- using doxygen format
- focus on the intent of the function/class - the view from the outside
- implementation details can be given inline in the code itself.

## classes
- names using CamelCase
- data members first and then any constructors
- private/protected methods appear after public methods
- use `override` for virtual functions in derived classes

Code should be written to be legible, comprehensible and testable. Writing unit tests in tandem with writing the product code can help highlight design issues early on, leading to early refactoring and a cleaner design.

The code guidelines [here](http://stroustrup.com/JSF-AV-rules.pdf) offer some good advice and we will in time be writing up which of these rules are applicale or not applicable to this codebase.
