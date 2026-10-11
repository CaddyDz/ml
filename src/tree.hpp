#ifndef TREE_HPP
#define TREE_HPP

#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include "node.hpp"

using namespace std;

/**
 * Expression tree. It owns its root node, hence every node of the tree.
 *
 * Next to the nodes it keeps indexes that build() recomputes from the root: the
 * node list (breadth-first), the depth, the nodes per depth, the heap positions
 * and the variables. The functions that modify the tree call build() themselves;
 * call it after linking nodes by hand (makeRoot, Node::s_left, ...).
 */
class Tree
{
private:
	Node* m_root=nullptr;
	unsigned int m_depth=0;
	std::vector<Node*> m_nodes;
	std::map<int,std::vector<Node*>> m_nodes_per_depth;
	std::map<int,Node*> m_node_at_pos;

	std::map<std::string, double> m_list_variables;

	double eval(Node*);
	bool shares_nodes_with(Node*) const;

public:
	Tree(){}
	~Tree();

	// The tree owns its nodes: copying it would free them twice.
	Tree(const Tree&) = delete;
	Tree& operator=(const Tree&) = delete;

	Node* root() const{return m_root;}                         ///< nullptr when the tree is empty
	unsigned int depth() const{return m_depth;}                ///< number of levels, 0 when empty
	const std::vector<Node*>& nodes() const{return m_nodes;}   ///< every node, breadth-first from the root
	const std::vector<Node*>& nodes_at_depth(int dp);          ///< empty when there is no node at that depth

	/// Heap numbering of the nodes: the root is at 0 and the children of position p
	/// at 2p+1 and 2p+2. Above the deepest level a missing child is stored as nullptr;
	/// the nodes of the deepest level have no child entries. Only the levels that fit
	/// in an int key are indexed, i.e. depths up to 31.
	const std::map<int,Node*>& nodes_from_pos() const{return m_node_at_pos;}

	// Printing functions
	void print();
	void print_variables();

	/// The function in the parser's syntax, e.g. "x^2-y+3" or "_exp[(-0.5)*x]":
	/// build_tree_from_equation() reads it back into the same tree (as long as the
	/// variable names are made of letters). Constants are written with 'precision'
	/// significant digits; an empty tree gives "".
	std::string to_equation(int precision=6) const;

	/// Independent deep copy of the tree (nodes, indexes and variable values).
	Tree* clone() const;

	// Miscellaneous operations
	bool isEmpty() const{return m_nodes.empty();}
	void clear(bool);   ///< empties the tree; the nodes are deleted only if the argument is true
	void build();       ///< recomputes every index from the root

	// Operations on tree
	bool find_node(Node*) const;   ///< true if the node is part of the tree

	// Search by value, written as in an equation: "4", "-2.5", "x", "+", "sin"
	Node* find_node_from_val_at_depth(std::string val, unsigned int dp);
	Node* find_node_from_val(std::string val);   ///< first match, breadth-first

	/// Sets the root of an empty tree (ignored if there is already one); call build() afterwards.
	void makeRoot(Node*);

	/// The node becomes the root and the former root its left operand, e.g. to wrap
	/// the whole function in a cosine. The node must be an operator or a function
	/// without a left child, with its right operand already set if it takes two
	/// (e.g. the 2 of (...)*2, set with s_right), and must not share nodes with any
	/// tree (use a clone() of another tree's subtree).
	/// Returns false and does nothing otherwise (the node stays the caller's).
	bool expand_from_root(Node*);

	/// Removes a node and puts its left (0) or right (1) child in its place;
	/// the other child is deleted with its subtree.
	void remove_intermediate(Node*, int=0);

	/// Replaces a node (deleted with its subtree) by another subtree, which the tree
	/// takes over. The new subtree must not share nodes with any tree: to copy a
	/// subtree from a tree (crossover), pass its clone(). Returns false and does
	/// nothing if the node to replace is not in the tree or the new subtree is
	/// refused (it then stays the caller's).
	bool replace(Node*,Node*);

	/// Stores a value for a variable. compute_value overwrites it with the values it
	/// is given, and requires a value for every stored name, including one the tree
	/// does not use, until the next build() drops it.
	void add_variable(std::string name,double val);

	/// Value of the function for the given values of its variables. Every variable
	/// of the tree must be given, otherwise the result is NaN (as for an empty tree).
	/// Division of a finite number by zero gives 0 (protected division).
	double compute_value(const std::map<std::string,double>&);
};

#endif // TREE_HPP
