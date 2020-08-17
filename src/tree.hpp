#ifndef TREE_HPP
#define TREE_HPP

#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include "node.hpp"

using namespace std;

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

public:
    Tree(){}
	~Tree();

    Node* root() const{return m_root;}
    unsigned int depth() const{return m_depth;}
    const std::vector<Node*>& nodes() const{return m_nodes;}
    const std::vector<Node*>& nodes_at_depth(int dp);
    const std::map<int,Node*>& nodes_from_pos() const{return m_node_at_pos;}

    // Printing functions
    void print();
    void print_variables();

    // Miscellaneous operations
    bool isEmpty() const{return m_nodes.empty();}
    void clear(bool);
    void build();

    // Operations on tree
    void addNode(Node* parent, Node* child);
    bool find_node(Node*) const;
    Node* find_node_from_val_at_depth(std::string val, unsigned int dp);
    Node* find_node_from_val(std::string val);
    void makeRoot(Node*);
    void expand_from_root(Node*);
    void remove_intermediate(Node*, int=0);
    void replace(Node*,Node*);

    void add_variable(std::string name,double val);

    // evaluate function behind tree
    double compute_value(const std::map<std::string,double>&);
};

#endif // TREE_HPP
