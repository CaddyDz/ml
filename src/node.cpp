#include "node.hpp"
#include <iostream>
#include <sstream>

Node::~Node()
{
    if (m_left) delete m_left;
    if (m_right) delete m_right;

    m_left = m_right = nullptr;
}

Node& Node::operator=(const Node& node)
{
    if (this !=&node)
    {
        m_left = node.left();
        m_right = node.right();
        m_depth = node.depth();
        m_type = node.type();
        m_parent = node.parent();

        if (m_left) m_left->s_depth(m_depth+1);
        if (m_right) m_right->s_depth(m_depth+1);
    }

    return (*this);
}

void Node::remove(Node* node)
{
	if (m_left == node)
		m_left = nullptr;
	else if (m_right == node)
		m_right = nullptr;
	
	return;
}

void Node::s_left(Node* left)
{
    m_left = left;
    if (left != nullptr)
    {
        m_left->s_depth(m_depth+1);
        m_left->s_parent(this);
    }
}

void Node::s_right(Node* right)
{
    m_right = right;

    if (right != nullptr)
    {
        m_right->s_depth(m_depth+1);
        m_right->s_parent(this);
    }
}

void Node::s_parent(Node* parent)
{
    m_parent = parent;
}

void Node::s_children(Node* left, Node* right)
{
    this->s_left(left);
    this->s_right(right);
}

int Node::which_node(Node* node) const
{
    if (node == m_left) return 0;
    else if (node == m_right) return 1;
    else return -1;
}

void Node::inc_depth()
{
    ++m_depth;
    if (m_left) m_left->inc_depth();
    if (m_right) m_right->inc_depth();
}

void Node::drop_depth()
{
    --m_depth;
    if (m_left) m_left->drop_depth();
    if (m_right) m_right->drop_depth();
}

void Node::s_depth(unsigned int dp)
{
    m_depth = dp;
    if (m_left) m_left->s_depth(m_depth+1);
    if (m_right) m_right->s_depth(m_depth+1);
}

void Node::removeAndFree()
{
	if (m_left != nullptr)
		delete m_left;
	
	if (m_right != nullptr)
		delete m_right;
	
	m_left = m_right = nullptr;
	
	return;
}

void Node::replace(Node* nodeToReplace, Node* node)
{
	if (m_left == nodeToReplace)
		m_left = node;
	else if (m_right == nodeToReplace)
		m_right = node;
	
	return;
}

void Node::rearrange()
{
	if (m_left == nullptr && m_right != nullptr)
		swap(m_left,m_right);
	return;
}

void Node::print() const
{
    cout << "PARENT " << this <<"            DEPTH: " << m_depth;
    cout << "\n        -- LEFT " << m_left <<" VALUE ";
    cout << "\n        -- RIGHT " << m_right << " VALUE " << endl;

	return;
}
