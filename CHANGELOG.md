# Changelog

## Genetic programming

Every generation used to be drawn at random, so nothing carried over from one
generation to the next. The classifier now evolves its population.

### Added

- Evolution in `CClassifier`: the first generation is drawn at random; each next one
  keeps the best trees (elitism) and breeds the others from parents chosen by
  tournament, by crossover or mutation. `evolve()` runs `s_nb_gen` generations;
  `s_nb_elite`, `s_tournament_size`, `s_crossover_rate` and `s_mutation_rate` tune
  it; `get_score()` and `nb_generations()` read the results.
- Variation operators in `CTreeGenerator`: `crossover()`, `mutate()` (subtree or
  single-node mutation) and `generate_subtree()`. The children follow the `Rules`
  and the parents are left untouched.
- Held-back test samples: `s_test_data()` adds samples that never take part in the
  evolution, and `get_test_score()` reports a tree's error on them, to check that a
  formula generalizes instead of fitting only its training points.
- Reproducible runs: `s_seed()` on the generator and on the classifier.
- `Tree::to_equation()` writes a tree in the parser's syntax (it parses back to the
  same tree), and `Tree::clone()` copies a tree.
- The demo evolves a formula for 37 samples of `x^2 - y + 3` with a little noise on
  the results (on exact samples, training and test errors were identical by
  construction), holds back 12 others to test it, prints both errors and draws the
  best formula; its seed can be given on the command line.
- Tests: formulas written and read back for random trees, the invariants of the
  variation operators, seeded replay, the evolution's invariants (sorted
  generations, a best score that never gets worse, no tree or node shared between
  generations), and held-back samples that leave a seeded run unchanged.

### Changed

- A tree's score is its root mean squared error (it was the square root of the sum
  of squared errors), so that training and test errors compare whatever the number
  of samples; the ranking of the trees is the same.
- A generation is sorted best first: `get_tree_for_specific_generation(0, g)` is the
  best tree, and `print_generation()` lists the trees in that order (they were in
  address order).
- The classifier is quiet: `s_verbose(true)` brings back the log of every tree drawn.
- A division by zero no longer prints a message at each evaluation.
- The parser reads a sign before a number as a negative constant (`-2.5` is the
  constant -2.5; it was `0-2.5`); values are unchanged.
- All the generator's random draws come from its own engine instead of the global
  `rand()`.

## Bug fixes and documentation

The fixes to the core (everything but the drawing) are covered by
`tests/test_core.cpp`. The demo was run with AddressSanitizer and
UndefinedBehaviorSanitizer, and the drawing was checked by rendering trees offscreen
with Qt 6.8, including with a dark palette.

### Build

- `parser.cpp` and `render.cpp` used `std::ostringstream` without including
  `<sstream>` (the deprecated `<strstream>` was included instead), so the project
  did not compile with libc++ (Apple clang).

### Crashes and memory errors

- The crossover demo grafted `tree7`'s root into `tree6`, so both trees deleted the
  same nodes when `main` returned (heap-use-after-free). It now grafts a copy made
  with the new `Node::clone()`.
- `Tree::find_node_from_val` used search predicates that did not return a value for
  nodes of another type (undefined behavior, the demo aborted under UBSan).
- `Tree::nodes_at_depth` returned a reference to a destroyed temporary for a depth
  absent from the tree.
- `Tree::replace` and `Tree::remove_intermediate` dereferenced a null parent when
  applied to the root; `remove_intermediate` also crashed when the child to keep did
  not exist (e.g. the right child of a one-argument function).
- `Tree::replace` accepted a node already in the tree (shared node, cycle or
  use-after-free); `Tree::expand_from_root` too, and it leaked the new root's
  children. Both now refuse any subtree that shares a node with the tree, and
  return whether they took the node over.
- The `tNode(OPERATOR, left, right)` constructor did not set the children's parent
  pointers, so mutating such a tree crashed.
- `CParser::parse` used an uninitialized pointer when an equation started with a
  sign (`-x`), contained a space or could not be parsed (`2x`, `x+`), and an empty
  equation passed a null root to the tree.
- The placeholders standing for groups and function calls (`expr0`, `func0`) could
  be typed in an equation or formed by accident (`(b)2` gave `expr12`); `(expr0)`
  recursed until the stack overflowed. They are now delimited by `#`, which
  equations may not contain.
- `Tree`, `CRender` and `CClassifier` could be copied although they own raw
  pointers (double delete); `Node`'s copy operations were broken (the copy
  constructor was never defined, `tNode::operator=` did not compile). They are now
  non-copyable, and `Node::clone()` makes deep copies.
- `CTreeGenerator` divided by zero with an empty variable list or a
  `max_polynomial_order` of 1.
- An out-of-range literal such as `1e999` made `std::stod` throw (abort).

### Hangs

- The parser looped forever on an unbalanced `(` or `[`, and on any equation with
  two function calls (`_sin[x]*_cos[x]`), whose greedy regex merged both calls.
- `Tree::build` visited 2^depth positions, so every extra level doubled the time to
  index a tree: a sum of 30 terms took about 3 s in an optimized build (over a minute
  in a debug build), and beyond about 33 levels the position count overflowed an
  `unsigned int` (undefined behavior). It is now linear in the number of nodes.
- `CTreeGenerator` could recurse without bound when `max_depth` was below 2.
- `CClassifier::compute_new_generation` never ended when no tree could be valid
  (e.g. a NaN reference result); it now gives up after 1000 x `s_nb_elm_per_gen`
  draws.

### Wrong results

- `_ln` computed log2 and `_log` the natural logarithm; they are now the natural
  and the base-10 logarithm.
- `^` was left-associative: `(1-5)^2^3` gave 4096 instead of 65536.
- Decimals were read with the C locale, which `QApplication` sets from the system:
  with a French locale `2.5` was read as 2.
- `_min` and `_max` could only take one argument and compared it with 0; they now
  take two: `_min[a,b]`.
- Unknown functions (`_tan[x]`) silently became `exp`, trailing operators
  (`x+1+`) were silently dropped and empty groups or calls were read as 0 (`x*()`
  gave 0, `_exp[]` gave 1); all are now reported as errors. Spaces now separate
  tokens (`x + 1`) but cannot split one (`x y` and `1e - 3` are errors).
- `*` and `/` were split in two passes, so `a*b/c` was computed as `a*(b/c)`; they
  now share one left-to-right level, like `+` and `-`.
- `Tree::build` and `Tree::clear` kept the depth and the variables of the previous
  tree, so a reused or mutated tree reported a wrong depth and required values for
  variables it no longer had.
- A missing variable value evaluated to 0 (and the classifier scored it); it is now
  NaN, which the classifier rejects. NaN now also survives a division by zero
  (which returned 0 for any numerator), `_min`/`_max` (which kept or dropped it
  depending on the argument order) and `^` (`pow(NaN, 0)` is 1), and an empty tree,
  a missing operand or an unknown variable evaluates to NaN instead of 0.
- `Tree::find_node_from_val(_at_depth)` guessed the node type from the text, so it
  could not find negative constants or variables whose names are not plain
  letters (`x1`), both of which the generator creates.
- `CTreeGenerator::check_rules` let negative and fractional exponents through (a
  fractional power of a negative number is NaN, a negative power of 0 infinite)
  and the random order it gives a non-constant exponent never reached
  `max_polynomial_order`; its replacement for a constant zero divisor was 0 again
  when the range of constants was `{0}`. The
  generator also accepted a range of constants that is not finite.
- Each random constant came from a newly seeded `std::random_device` engine; where
  `random_device` is deterministic (MinGW before GCC 9.2, Qt's default Windows
  toolchain) every constant had the same value. The generator now keeps one engine,
  seeded once.
- Two generators created within the same second drew trees of the same shapes, only
  the constants differing (the random seed was reset in every constructor).
- `CClassifier::get_tree_for_specific_generation` picked the tree by the
  generation index, and both its bounds checks and `print_generation`'s were off by
  one.
- `CClassifier::compute_new_generation` leaked every rejected tree, logged the sum
  of squared errors while storing its square root, and gave an infinite score to
  trees whose sum of squared errors overflowed. The score is now accumulated
  without overflow (`std::hypot`), and a tree whose score is still infinite is
  discarded.
- Demo: the fifth sample of `x^2 - y + 3` had the result 3 instead of 2, and the
  first evaluation was labelled `x=3` while computed with `x=-5`.

### Rendering

- Deep trees were drawn with overlapping nodes and cut at the bottom of the scene.
  Nodes are now laid out in order (one column each) and the scene fits the drawing.
- Long labels, such as the generator's constants, overflowed their 40px node.
  Nodes now widen to fit their label (ellipses, and a rectangle for the root),
  labels are centered and shapes use floating-point rectangles.
- With a dark theme the black shapes and lines were drawn on a dark background;
  the drawing is now always black on white.
- Drawing a tree again with the same `CRender` superimposed both drawings; the new
  drawing now replaces the previous one (an empty tree clears it), and `drawNode`
  grows the scene so that its drawing is not cut off.

### API cleanup

- Removed declarations that were never defined, so calling them could only fail to
  link: `Tree::addNode`, `CParser::parse_simple_operation`,
  `CParser::build_equation_from_tree`. Removed `tNode::eval()`, which always
  returned 1.0 (use `Tree::compute_value`).
- Implemented the declared `CData::get_data(name)` (NaN for an unknown variable);
  `CData::s_result` now takes the result to set, and the constructors take it by
  value (an lvalue did not compile).
- `Tree::expand_from_root` keeps the right child of the new root, so an operator
  with its right operand set can wrap the tree, e.g. into `(...)*2`.
- `CRender::drawNode(node, x, y)`: `(x, y)` is now the top left corner of the
  subtree's drawing (it was the position of the node itself).

### Documentation and tests

- README rewritten: build, tests, concepts, equation syntax, usage, limitations.
- API comments in every header.
- New headless regression tests in `tests/` (`make check`), run with
  AddressSanitizer (including stack-use-after-return) and UndefinedBehaviorSanitizer;
  `make leaks` checks for leaks on macOS, where LeakSanitizer is unavailable.
- `.gitignore` covers in-source qmake builds, Qt Creator's shadow builds and the
  test binaries.

### Not changed (on purpose)

- Division by zero still returns 0 for a finite numerator: this "protected
  division" is a common genetic-programming convention that keeps random trees
  evaluable.
- `CONFIG += c++1y` stays in `Tree.pro`: the code is C++14, and Qt 6's qmake raises
  the standard to C++17 by itself. `M_PI` in `main.cpp` is provided by Qt's
  `qmath.h` when the platform lacks it.
- The node type is still set by hand with `s_type()`, and `Rules` has no default
  values: it is meant to be filled in full, as in the README.
- `check_rules()` only replaces a constant divisor equal to 0; other divisions by
  zero are handled by the protected division at evaluation time.
- `Tree::add_variable()` keeps its behavior: `compute_value()` overwrites the stored
  values with the ones it is given, and requires one for a name the tree does not
  use until the next `build()` drops it.
- `Node::print()` (debug output) still prints addresses only, not values.
- Style or performance only, left as is: `using namespace std` in headers, the
  lowercase layout macros in `render.hpp`, the unused Qt Designer `widget.*` stub,
  the enum sizes hard-coded in the generator's random choices.
