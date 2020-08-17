This release includes:

- The definition of the Tree and Node structures to build a tree
- The classic features expected for building and modifying a tree: Crossover, Mutation (substitution, remove,), Reducing complexity of syntax trees ...
- The definition of a Data structure, which makes it possible to store data to be fitter
- The definition of a Rules structure which allows to store rules for the generation of a tree.
- The definition of a TreeGenerator structure, which allows random generation from predefined Rules.
- The definition of a Classifier structure, which makes it possible to generate N trees per generation and calculate the error (by least squares) compared to a series of reference data (contained in a Data structure).

The code also defines:

- A Parser structure, which allows you to parse a string containing an equation and transform it into a tree
- A Render structure, which uses Qt functionality to display the tree in a GraphicsScene. Note that this function still contains work, because if the display is successful, a tall tree sees some of these nodes overlap, making viewing difficult
