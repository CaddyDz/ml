# Tree — expression trees for symbolic regression

A small C++/Qt toolkit for genetic programming on mathematical functions. A function
such as `3 + 4*exp(3+x)` is stored as a binary expression tree, which can be

- built by hand or **parsed** from an infix equation,
- **evaluated** for given values of its variables,
- **mutated** (replace, remove or wrap nodes) and **crossed over** with another tree,
- **generated at random** from a set of rules,
- **evolved** to fit reference data (symbolic regression): a population of trees is
  improved generation after generation by tournament selection, crossover,
  mutation and elitism,
- **written** back as an equation and **drawn** with Qt.

`src/main.cpp` is a demo that walks through every feature: it prints its results
to the console, opens one window per tree, and ends by evolving a formula for 25
samples of `x^2 - y + 3`.

## Contents

| Files | What it holds |
| --- | --- |
| `defs.hpp/.cpp` | Node kinds, operators and functions, their text tables, token predicates |
| `node.hpp/.cpp` | `Node` (links to children and parent, depth) and `tNode<T>` (the value) |
| `tree.hpp/.cpp` | `Tree`: owns the nodes; indexes, search, mutations, evaluation |
| `parser.hpp/.cpp` | `CParser`: infix equation → tree |
| `treegenerator.hpp/.cpp` | `CTreeGenerator`: random trees following `Rules`, crossover and mutation |
| `data.hpp/.cpp` | `CData`: one reference sample (variable values and expected result) |
| `classifier.hpp/.cpp` | `CClassifier`: the evolution, generations of trees scored against the samples |
| `render.hpp/.cpp` | `CRender`: draws a tree in a `QGraphicsScene` |
| `main.cpp` | Demo application |
| `widget.*` | Qt Designer stub created with the project (not used) |
| `../tests/` | Headless regression tests of everything but the rendering |

## Building

### Demo application

Requires Qt with the Widgets module and a C++14 compiler. Built and tested with
Qt 6.8 (whose qmake switches to C++17 by itself); Qt 5.11 or later should work too.

```sh
cd src
qmake Tree.pro
make            # nmake or jom with MSVC
```

Or open `src/Tree.pro` in Qt Creator.

With Qt 6.8 and a recent macOS SDK, two known Qt/SDK incompatibilities can show
up (they are not specific to this project, and a newer Qt may not have them):

- `implicitly declaring library function '__yield'` in `qyieldcpu.h`: run
  `qmake Tree.pro "QMAKE_CXXFLAGS+=-include arm_acle.h"`;
- `ld: framework 'AGL' not found`: the SDK no longer ships AGL; delete
  `-framework AGL` from the generated `Makefile`.

### Tests

The tests only need a C++ compiler and GNU make (`gmake` on BSD), no Qt:

```sh
cd tests
make check
```

They are built with AddressSanitizer and UndefinedBehaviorSanitizer; with a
compiler that lacks them run `make check SANITIZE=`. Choose the compiler with
`make check CXX=clang++`. On macOS, where LeakSanitizer is not available,
`make leaks` runs the tests under the system `leaks` tool and fails if anything
leaks. The decimal-comma test needs a `fr_FR` or `de_DE` locale; without one it
prints a notice and only checks the C locale.

## Core concepts

### Nodes

Every node is a `tNode<T>`; its `NodeType` tells which `T`:

| `NodeType` | Class | Value | Children |
| --- | --- | --- | --- |
| `CONSTANT` | `tNode<double>` | number | none |
| `VARIABLE` | `tNode<std::string>` | variable name | none |
| `OPERATOR` | `tNode<OPERATOR>` | `ADD MINUS PROD DIV POW` | left and right operands |
| `FUNCTION` | `tNode<FUNCTION>` | `EXP LN LOG ROOTSQUARE COS SIN MIN MAX` | the argument on the left (`MIN`/`MAX`: two arguments) |

The type is not deduced from `T`: set it with `s_type()` after creating a node
(only the `tNode<OPERATOR>(op, left, right)` constructor does it for you).

**Ownership.** A node owns its children — deleting a node deletes its subtree —
and a `Tree` owns its root. Nodes and trees cannot be copied; `Node::clone()` makes
an independent deep copy of a subtree.

**Depth.** The root is at depth 1. Linking a child with `s_left`, `s_right` or
`s_children` sets its parent and the depths of its whole subtree.

### Tree indexes

Besides the nodes, a `Tree` keeps indexes computed by `build()`: the node list
(breadth-first), the depth, the nodes per depth, the heap positions
(`nodes_from_pos()`) and the variables. `replace`, `remove_intermediate` and
`expand_from_root` rebuild them; call `build()` yourself after linking nodes by
hand.

## Equation syntax

| Element | Examples | Notes |
| --- | --- | --- |
| Numbers | `3`, `2.5`, `.5`, `1e3` | a signed exponent must be grouped: `(1e-3)` |
| Variables | `x`, `theta` | letters only, and not a function name |
| Operators | `+ - * / ^` | `^` binds tightest, then `*` and `/`, then `+` and `-` |
| Sign | `-x`, `(-3)^2`, `2*(-x)` | only at the start of an expression or a group: `2*-x` is rejected |
| Groups | `(x+1)*2` | |
| Functions | `_exp[x]`, `_ln[x]`, `_log[x]`, `_sqrt[x]`, `_sin[x]`, `_cos[x]` | square brackets; `ln` is the natural log, `log` the base-10 one |
| Two-argument functions | `_min[x,2]`, `_max[x,y]` | |

`+ - * /` are left-associative (`10-4-3` is 3, `8/4*2` is 4) and `^` is
right-associative (`2^2^3` is 2^8 = 256). A sign before a number makes a negative
constant (`(-2.5)`); before anything else, `-x` is read as `0-x`. Spaces may separate tokens (`x + 1`) but
not split one: `x y` is an error, not the variable `xy`. The character `#` is
reserved. An invalid equation prints the reason on stderr and leaves the tree empty.

`Tree::to_equation()` writes a tree in this syntax, e.g. to print an evolved formula;
parsing that text builds the same tree again (as long as the variable names are made
of letters).

## Evaluation

`Tree::compute_value(values)` evaluates the function. NaN means "no valid value"
and propagates through every operation:

- every variable of the tree needs a value, otherwise the result is NaN (an empty
  tree, e.g. after a parse error, is NaN too);
- dividing a finite number by zero returns 0 (the usual "protected division" of
  genetic programming); dividing NaN or an infinity by zero keeps it;
- functions follow `<cmath>`: `_sqrt` and the logarithms of negative numbers give
  NaN, and `_min`/`_max` return NaN if either argument is NaN.

## Usage

```cpp
#include "parser.hpp"

CParser parser;
Tree tree;
parser.build_tree_from_equation(&tree, "3+4*((y+2*x+t)^2-z^3+_sin[theta])");

double value = tree.compute_value({{"x", -5}, {"y", 2}, {"z", 1}, {"t", -1}, {"theta", 0.5}});
```

Building a tree by hand:

```cpp
Node* four = new tNode<double>(4.0);
four->s_type(NodeType::CONSTANT);
Node* y = new tNode<std::string>("y");
y->s_type(NodeType::VARIABLE);

Tree tree;
tree.makeRoot(new tNode<OPERATOR>(OPERATOR::ADD, four, y));   // 4+y
tree.build();                                                   // mandatory after makeRoot
```

Mutations and crossover:

```cpp
// mutation: change an operator in place
Node* op = tree.find_node_from_val("^");
static_cast<tNode<OPERATOR>*>(op)->s_data(OPERATOR::PROD);

// remove a node, keeping its left (0) or right (1) child in its place
tree.remove_intermediate(tree.find_node_from_val("cos"), 0);

// wrap the whole function: the new node becomes the root
Node* cos = new tNode<FUNCTION>(FUNCTION::COS);
cos->s_type(NodeType::FUNCTION);
tree.expand_from_root(cos);

// crossover: graft a COPY of a subtree of another tree
Node* gene = tree.find_node_from_val_at_depth("+", 3);
Node* graft = other.root()->clone();
if (!tree.replace(gene, graft))
    delete graft;           // refused (e.g. no gene found): still ours
```

`replace` deletes the replaced subtree and takes over the new one, which must not
share nodes with any tree: grafting `other.root()` itself would leave both trees
owning (and deleting) the same nodes. Like `expand_from_root`, it returns false
when it refuses, and the node then stays yours.

Random trees and evolution:

```cpp
Rules rules{4, 2, {"x", "y"}, -5, 5};   // max depth, max exponent, variables, constant range
CTreeGenerator generator(rules);
Tree* random_tree = generator.generate_tree();   // owned by the caller
Tree* mutant = generator.mutate(*random_tree);   // a new tree, owned by the caller too

CData sample({{"x", 1.0}, {"y", -1.0}}, 5);   // x=1, y=-1 -> expected 5 (sample.get_data("x") is 1)
CData held_back({{"x", 2.0}, {"y", 0.0}}, 7);
CClassifier classifier;
classifier.s_data(&sample);                    // add many samples; they are not owned by the classifier
classifier.s_test_data(&held_back);             // samples kept out of the evolution, to test the result
classifier.s_nb_elm_per_gen(200);
classifier.s_nb_gen(40);
generator.s_seed(1);                            // optional: replay the same run
classifier.s_seed(1);
classifier.evolve(&generator);

int last = classifier.nb_generations() - 1;
Tree* best = classifier.get_tree_for_specific_generation(0, last);   // rank 0 is the best
std::cout << best->to_equation() << " training error " << classifier.get_score(0, last)
          << " test error " << classifier.get_test_score(0, last) << std::endl;
```

Drawing (Qt):

```cpp
CRender render;
render.drawTree(&tree);
QGraphicsView view(render.scene());
view.show();
```

## Random generation

`CTreeGenerator` follows `Rules`:

| Field | Meaning |
| --- | --- |
| `max_depth` | deepest level, at least 2: the root is always an operator, the nodes at `max_depth` are leaves |
| `max_polynomial_order` | largest exponent allowed after `^`, at least 1 |
| `m_list_variables` | names the variable leaves are drawn from (may be empty) |
| `min_number`, `max_number` | range of the constant leaves |

After drawing a tree, `check_rules()` makes every exponent a whole number between 1
and `max_polynomial_order`, and replaces a division by the constant 0.

The generator also provides the variation operators of the evolution. Both return a
new tree that follows the rules and leave their parents untouched:

- `crossover(mother, father)`: a copy of the mother in which a random subtree is
  replaced by a copy of a random subtree of the father;
- `mutate(parent)`: a copy of the parent in which, half of the time, a random subtree
  is replaced by a new random one, otherwise a single node changes (another operator,
  another function of the same arity, another variable, or a constant moved by a
  small normal step).

Crossover and mutation pick an internal node 90% of the time. A child deeper than
`max_depth` is drawn again.

## Evolution

`CClassifier` fits a formula to the samples by genetic programming. A tree's score
is its root mean squared error (RMSE) over the training samples given to `s_data`
(lower is better); a tree that gives NaN or an infinity for a sample is invalid and
discarded.

- The first `compute_new_generation()` draws `s_nb_elm_per_gen` random valid trees.
- Each following call breeds the next generation from the previous one: the
  `s_nb_elite` best trees are copied unchanged, and each other tree comes from
  parents chosen by tournament (the best of `s_tournament_size` random trees), by
  crossover (probability `s_crossover_rate`), mutation (`s_mutation_rate`) or a plain
  copy.
- `evolve()` computes `s_nb_gen` generations at once.

| Setting | Default |
| --- | --- |
| `s_nb_elm_per_gen` (population) | 100 |
| `s_nb_gen` (generations of `evolve`) | 1 |
| `s_nb_elite` | 2 |
| `s_tournament_size` | 4 |
| `s_crossover_rate` / `s_mutation_rate` | 0.7 / 0.25 |

A generation is sorted best first: `get_tree_for_specific_generation(0, g)` and
`get_score(0, g)` give the best tree of generation `g` and its score, and thanks to
elitism the best score never gets worse from one generation to the next. A
generation can hold fewer trees if valid ones are too rare (it gives up after 1000 x
`s_nb_elm_per_gen` draws, e.g. when a sample's result is NaN). The classifier owns its
trees; `s_verbose(true)` logs every tree drawn.

### Checking that a formula generalizes

A low training error alone does not show that a formula is right: it may only fit
the points it was evolved on (overfitting). Samples given to `s_test_data` are held
back: they never take part in the scoring or the selection, and
`get_test_score(rank, generation)` gives a tree's RMSE on them. A test error close to
the training error means the formula holds beyond its training points; a much larger
one means it memorized them. Report the test error, but do not pick formulas by it:
the test samples would then take part in the selection.

Two cautions when reading the numbers. On samples computed exactly from a function,
a formula that is off by the same amount everywhere, like `x^2+3.002-y` for
`x^2+3-y`, has exactly the same error on any set of points: identical training and
test errors show nothing about generalization. Such samples also leave nothing to
memorize, as long as there are enough of them to pin the formula down. The demo adds
noise to its samples so that its two errors are not identical by construction; it
still does not overfit (see its results below), while the snippet in
[Usage](#usage), evolved on a single point, does. Identical numbers can also look
like test samples leaking into the training: the `held-back samples` test checks
that a seeded run evolves exactly the same formulas with and without test samples.

### Reproducible runs

Seeding the generator and the classifier (`s_seed`) replays a run exactly, on a
given standard library (libc++ and libstdc++ draw different numbers from the same
seed). The demo takes the seed as its argument, `./Tree 4` (on macOS
`./Tree.app/Contents/MacOS/Tree 4`), and prints the one it used otherwise.

### Results of the demo

The demo samples `x^2 - y + 3` on a 7x7 grid, adds a little noise to every result
(normal, standard deviation 0.1, as measured data would have), holds back a random
quarter of the 49 points (12) for testing, and evolves 40 generations of 200 trees
with the rules `{4, 2, {x, y}, -5, 5}`. A line is printed when the best training
error improves (formulas are not simplified). Seed 4, on macOS (libc++):

```
Evolving a formula for x^2-y+3 from 37 noisy samples, 12 others held back for testing (seed 4)
Generation 1: training error 2.23661, test error 2.18982  2.30153+(x^2-_cos[y])
Generation 2: training error 2.0633, test error 1.86882  3.03235+x^2
Generation 3: training error 2.0595, test error 1.92821  2.77386+x^2
Generation 6: training error 0.683905, test error 0.708184  2.30153+(x^2-y)
Generation 8: training error 0.0938181, test error 0.0768367  3.03235+(x^2-y)
Generation 40: training error 0.0892964, test error 0.0742825  3.02354+(x^2-y)
```

With noise of 0.1 on every result, an error of about 0.1 is the best a formula can
do without fitting the noise, and the final formula is there on both sets. Over
seeds 1 to 20 with the same settings, 17 runs end with the shape `x^2 - y + c`
(`c` between 2.70 and 3.11), with training errors between 0.09 and 0.31 and test
errors between 0.07 and 0.25; the test error is the larger of the two in 10 runs
and the smaller in 7. Seed 20 finds `x^2-(y-2-1)`, the function itself, and still
scores 0.099 on its training points and 0.090 on the test points: the noise. No run
scores well below the noise on its training points, which is what fitting their
noise would look like; a tree of depth 4 has too few constants to fit 37 points.
The other three of the 20 runs stall: seeds 5 and 18 miss `y`
(`_max[0.994227,x^2]+2.89005`, training error 1.91, test error 2.34;
`x^2+_max[x,2.56247]/0.906928`, 2.02 and 2.01) and seed 16 gives `y` the wrong
weight (`3.13174+(x^2+y*(-0.725104))`, 0.48 and 0.74).

The snippet in [Usage](#usage), which trains on a single point, does overfit: with
seed 1 it ends with `(-4.99886)/y`, training error 0.001, test error 7 (`y` is 0 at
the held-back point, where the protected division gives 0).

## Known limitations

- The evolution has a single objective, the error: there is no constant optimization
  (constants only move by mutation steps), no simplification of the formulas, and no
  pressure towards small trees beyond `max_depth` (equal scores prefer the smaller
  tree). A run can stall on a local optimum; another seed or a larger population helps.
- Every generation stays in memory until the classifier is destroyed: a long run with
  a large population takes a lot of memory.
- The parser has no implicit multiplication (`2x`), and a sign is only accepted at
  the start of an expression or a group.
- `nodes_from_pos()` only indexes the first 31 levels.
- Parsing, evaluating, copying and deleting are recursive: trees or equations
  nested thousands of levels deep can overflow the stack.
- The drawing gives each node its own column, so a large tree is drawn wide.
