#ifndef TREEGENERATOR_HPP
#define TREEGENERATOR_HPP

#include <random>
#include <cstdlib>
#include "tree.hpp"

/// Constraints of the random trees
typedef struct
{
	unsigned int max_depth;                     ///< deepest level, at least 2 (the root is an operator)
	unsigned int max_polynomial_order;          ///< largest exponent after ^, at least 1
	std::vector<std::string> m_list_variables;  ///< names of the variable leaves
	double min_number;                          ///< range of the constant leaves
	double max_number;

} Rules;

/**
 * Random trees following Rules, and the variation operators of genetic programming.
 *
 * generate_tree(): the root is an operator, the nodes below are drawn among
 * constants, variables, operators and functions, and the nodes at max_depth are
 * leaves (constants or variables). check_rules() then makes every exponent a valid
 * polynomial order and replaces divisions by a constant 0.
 *
 * crossover() and mutate() build a new tree from one or two parents, which they
 * leave untouched; the child follows the Rules too. All the randomness comes from
 * one engine per generator: s_seed() makes a run reproducible (on a given standard
 * library: libc++ and libstdc++ draw different numbers from the same seed).
 */
class CTreeGenerator
{
private:
	Rules m_rules;
	mutable std::mt19937 m_engine;
	void generate_node(Node*&, unsigned int) const;
	double random_constant() const;
	std::size_t random_index(std::size_t) const;
	Node* pick_node(const Tree&) const;
	void point_mutation(Node*) const;

public:
	CTreeGenerator() = delete;
	CTreeGenerator(const Rules&);
	CTreeGenerator(const CTreeGenerator&) = delete;
	~CTreeGenerator(){}

	Rules rules() const{return m_rules;}
	void s_seed(unsigned int seed){m_engine.seed(seed);}   ///< restarts the random sequence
	Tree* generate_tree() const;     ///< new tree, to be deleted by the caller
	bool check_rules(Tree*) const;   ///< fixes the tree as described above; false if it is null or too deep

	/// Random subtree whose root sits at the given depth of a tree: its leaves stop at
	/// max_depth. Owned by the caller (e.g. to pass to Tree::replace).
	Node* generate_subtree(unsigned int depth) const;

	/// Subtree crossover: a copy of the mother in which a random node is replaced by a
	/// copy of a random node of the father (internal nodes are picked 90% of the time).
	/// A child deeper than max_depth is drawn again; after 10 tries the mother is copied.
	Tree* crossover(const Tree& mother, const Tree& father) const;

	/// Mutation of a copy of the parent, at a random node: half of the time the node's
	/// subtree is replaced by a random one, otherwise the node itself changes (another
	/// operator, another function of the same arity, another variable, or a constant
	/// moved by a normal step of 10% of its size, at least 0.1).
	Tree* mutate(const Tree& parent) const;
};

#endif // TREEGENERATOR_H
