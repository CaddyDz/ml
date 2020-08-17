#ifndef NODE_HPP
#define NODE_HPP

#include <utility>
#include <string>
#include <iostream>
#include <typeinfo>
#include "defs.hpp"

using namespace std;

class Node
{

protected:
   NodeType m_type=NodeType::NONE;

   Node* m_left = nullptr;
   Node* m_right = nullptr;
   Node* m_parent = nullptr;
   unsigned int m_depth=1;

public:
   Node(){}
   virtual ~Node();
   Node(const Node&);

   Node& operator=(const Node&);

   void inc_depth();
   void drop_depth();

   NodeType type() const{return m_type;}
   Node* left() const{return m_left;}
   Node* right() const{return m_right;}
   Node* parent() const{return m_parent;}
   unsigned int depth() const{return m_depth;}
   void s_left(Node* left);
   void s_right(Node* right);
   void s_children(Node* left, Node* right);
   void s_parent(Node* parent);
   void s_depth(unsigned int);
   void s_type(NodeType type){m_type=type;}

   bool isFull(){return ((m_left != nullptr) && (m_right != nullptr));}
   void remove(Node*);
   int which_node(Node* node) const;
   void removeAndFree();
   void replace(Node*, Node*);
   void rearrange();

   virtual void print() const;
};


template <typename T> class tNode : public Node
{
private:

    T m_data;

public:
    tNode<T>():Node(){}
    tNode<T>(const tNode<T>& node):Node(node)
    {
        m_data = node.data();
    }

    virtual ~tNode<T>(){}

    tNode<T>& operator=(const tNode<T>& node)
    {
        if (this != node)
        {
            Node::operator=(node);
            m_data = node.data();
        }

        return (*this);
    }

    tNode<T>(const OPERATOR& op, Node* left, Node* right)
    {
        m_type = NodeType::OPERATOR;
        m_left = left;
        m_right = right;
        m_data = op;

        this->rearrange();

        if (m_left) m_left->inc_depth();
        if (m_right) m_right->inc_depth();

    }

    tNode<T>(const T& data){
        m_data = data;
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

    double eval() const
    {
      return 1.0;


    }
};

#endif // NODE_HPP

