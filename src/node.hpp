#ifndef NODE_HPP
#define NODE_HPP

#include <utility>
#include <string>
#include <iostream>
#include <typeinfo>
#include "defs.hpp"

using namespace std;

/**
 * Base class of the expression tree nodes: links (left/right children, parent)
 * and depth. The value lives in tNode<T>, type() tells which T to cast to.
 *
 * A node owns its children: deleting a node deletes its whole subtree. The depth
 * of a root is 1; s_left/s_right/s_children/s_depth keep the depths below a node
 * up to date.
 */
class Node
{

protected:
   NodeType m_type=NodeType::NONE;

   Node* m_left = nullptr;
   Node* m_right = nullptr;
   Node* m_parent = nullptr;
   unsigned int m_depth=1;

   void copy_into(Node*) const;

public:
   Node(){}
   virtual ~Node();

   // A node owns (and deletes) its children, so a copy would share them and free
   // them twice. Use clone() to duplicate a subtree.
   Node(const Node&) = delete;
   Node& operator=(const Node&) = delete;

   /// Deep copy of the subtree rooted at this node (the copy has no parent).
   virtual Node* clone() const;

   void inc_depth();   ///< adds 1 to the depth of every node of the subtree
   void drop_depth();  ///< removes 1 from the depth of every node of the subtree

   NodeType type() const{return m_type;}
   Node* left() const{return m_left;}
   Node* right() const{return m_right;}
   Node* parent() const{return m_parent;}
   unsigned int depth() const{return m_depth;}

   // Setting a child also sets its parent and the depths of its subtree.
   // The previous child is not deleted.
   void s_left(Node* left);
   void s_right(Node* right);
   void s_children(Node* left, Node* right);
   void s_parent(Node* parent);
   void s_depth(unsigned int);              ///< sets this depth and those of the subtree below
   void s_type(NodeType type){m_type=type;} ///< must match the T of tNode<T>, see NodeType

   bool isFull(){return ((m_left != nullptr) && (m_right != nullptr));}
   void remove(Node*);                ///< forgets the given child (it is not deleted)
   int which_node(Node* node) const;  ///< 0 for the left child, 1 for the right one, -1 otherwise
   void removeAndFree();              ///< deletes both children with their subtrees
   void replace(Node*, Node*);        ///< swaps a child pointer only: no delete, parent or depth update
   void rearrange();                  ///< moves a lone right child to the left

   virtual void print() const;        ///< debug output: addresses and depth
};


/**
 * Node holding a value of type T:
 *   tNode<double>       a constant  (NodeType::CONSTANT), leaf
 *   tNode<std::string>  a variable  (NodeType::VARIABLE), leaf
 *   tNode<OPERATOR>     an operator (NodeType::OPERATOR), two children
 *   tNode<FUNCTION>     a function  (NodeType::FUNCTION), left child (and right child for min/max)
 * The NodeType is not deduced from T: call s_type() after creating a node.
 */
template <typename T> class tNode : public Node
{
private:

	T m_data{};

public:
	tNode():Node(){}

	virtual ~tNode(){}

	/// Operator with its operands (T must be OPERATOR). Unlike the other
	/// constructors it sets the type itself. A lone right operand is moved to the
	/// left (rearrange()); to build an operator with only its right operand, use
	/// tNode<OPERATOR>(op), s_type() and s_right().
	tNode(const OPERATOR& op, Node* left, Node* right)
	{
		m_type = NodeType::OPERATOR;
		m_data = op;

		// s_children also sets the parent pointer and the depth of each child
		this->s_children(left, right);
		this->rearrange();
	}

	tNode(const T& data){
		m_data = data;
		}

	virtual Node* clone() const
	{
		tNode<T>* copy = new tNode<T>(m_data);
		this->copy_into(copy);
		return copy;
	}

	/*
	 * Node operations
	 */

	T data() const{return m_data;}
	inline void s_data(T val){m_data = val;}
	virtual void print() const
	{
		Node::print();
		return;
	}
};

#endif // NODE_HPP

