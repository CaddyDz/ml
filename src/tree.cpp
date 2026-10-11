#include "tree.hpp"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <unordered_set>

Tree::~Tree()
{
	if (m_root) delete m_root;
}

void Tree::clear(bool force_delete)
{
	if (force_delete)
		if (m_root) delete m_root;

	m_root = nullptr;
	m_depth = 0;
	m_nodes.clear();
	m_nodes_per_depth.clear();
	m_node_at_pos.clear();
	m_list_variables.clear();
}

bool Tree::find_node(Node* node_to_find) const
{
	auto it = std::find(m_nodes.begin(), m_nodes.end(), node_to_find);

	if (it != m_nodes.end())
		   return true;

	return false;

}

const std::vector<Node*>& Tree::nodes_at_depth(int dp)
{
	static const std::vector<Node*> no_nodes;

	auto it = m_nodes_per_depth.find(dp);

	if (it != m_nodes_per_depth.end())
		return it->second;
	else
		return no_nodes;

}

namespace
{
	// true if the node holds the value written as in an equation ("4", "-2.5", "x",
	// "+", "sin"). The node's own type decides how the text is read, so negative
	// constants and any variable name (e.g. "x1" from the generator) can be found.
	bool holds_value(Node* node, const std::string& val)
	{
		switch (node->type())
		{
		case NodeType::CONSTANT:
		{
			bool sign = !val.empty() && (val[0] == '-' || val[0] == '+');
			std::string number = sign ? val.substr(1) : val;

			if (!is_number(number))
				return false;

			double value = (val[0] == '-') ? -to_number(number) : to_number(number);
			return static_cast<tNode<double>*>(node)->data() == value;
		}

		case NodeType::VARIABLE:
			return static_cast<tNode<std::string>*>(node)->data() == val;

		case NodeType::OPERATOR:
		{
			auto op = str2op.find(val);
			return op != str2op.end() && static_cast<tNode<OPERATOR>*>(node)->data() == op->second;
		}

		case NodeType::FUNCTION:
		{
			auto fn = str2fn.find(val);
			return fn != str2fn.end() && static_cast<tNode<FUNCTION>*>(node)->data() == fn->second;
		}

		default:
			return false;
		}
	}
}

Node* Tree::find_node_from_val(std::string val)
{
	auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
						   [&val](Node* node){ return holds_value(node, val); });

	return (it != m_nodes.end()) ? *it : nullptr;
}

Node* Tree::find_node_from_val_at_depth(std::string val, unsigned int dp)
{
	const std::vector<Node*>& nodes = this->nodes_at_depth(dp);

	auto it = std::find_if(nodes.begin(), nodes.end(),
						   [&val](Node* node){ return holds_value(node, val); });

	return (it != nodes.end()) ? *it : nullptr;
}

void Tree::makeRoot(Node* root)
{
	if (m_root != NULL)
	{
		std::cerr << "WARNING: the tree already has a root node, makeRoot() ignored. Call clear() first." << std::endl;
		return;
	}

	m_root = root;
	if (m_root) m_root->s_depth(1);
}

bool Tree::shares_nodes_with(Node* subtree) const
{
	// true if a node of the subtree is a node of this tree
	std::unordered_set<Node*> own(m_nodes.begin(), m_nodes.end());
	std::vector<Node*> to_visit{subtree};

	while (!to_visit.empty())
	{
		Node* node = to_visit.back();
		to_visit.pop_back();

		if (node == nullptr)
			continue;

		if (own.count(node))
			return true;

		to_visit.push_back(node->left());
		to_visit.push_back(node->right());
	}

	return false;
}

bool Tree::expand_from_root(Node* node)
{
	if (node == nullptr)
		return false;

	bool binary = (node->type() == NodeType::OPERATOR);

	if (node->type() == NodeType::FUNCTION)
	{
		FUNCTION fn = static_cast<tNode<FUNCTION>*>(node)->data();
		binary = (fn == FUNCTION::MIN || fn == FUNCTION::MAX);
	}

	// the former root becomes the left operand of the node: it needs a tree to wrap,
	// an operator or a function with a free left side and the right operand it takes
	// (if any), and no node of the tree in its subtree (that would create a cycle)
	if (m_root == nullptr || node->left() != nullptr ||
		(node->type() != NodeType::OPERATOR && node->type() != NodeType::FUNCTION) ||
		binary != (node->right() != nullptr) || this->shares_nodes_with(node))
	{
		std::cerr << "Tree::expand_from_root: the new root must be an operator or a function with no left child and its right operand if it takes two, outside the tree. Nothing done." << std::endl;
		return false;
	}

	node->s_left(m_root);

	m_root = node;
	m_root->s_parent(nullptr);
	m_root->s_depth(1);

	// rebuild the tree
	this->build();

	return true;
}

void Tree::print()
{
	std::cout << "TREE DEPTH: "<<m_depth <<std::endl;
	std::cout << "NB NON-ZERO NODES: "<<m_nodes.size() <<std::endl;

	std::for_each(m_nodes.begin(), m_nodes.end(),[](Node* node)
				  {
					std::cout << node;
					if (node != nullptr)
					{
						 std::cout << " " << node->depth() << " " << (int)node->type() << " ";

						 switch(node->type())
						 {

						 case NodeType::CONSTANT:
						 {
							 tNode<double>* tnode = static_cast<tNode<double>*>(node);
							 std::cout << tnode->data();
							 break;
						 }

						 case NodeType::OPERATOR:
						 {
							 tNode<OPERATOR>* tnode = static_cast<tNode<OPERATOR>*>(node);
							 std::cout << op2str[tnode->data()];
							 break;
						 }

						 case NodeType::FUNCTION:
						 {
							 tNode<FUNCTION>* tnode = static_cast<tNode<FUNCTION>*>(node);
							 std::cout << fn2str[tnode->data()];
							 break;
						 }

						 case NodeType::VARIABLE:
						 {
							 tNode<std::string>* tnode = static_cast<tNode<std::string>*>(node);
							 std::cout << tnode->data();
							 break;
						 }

						 default:
							break;

						 }
					}

					std::cout << std::endl;

				  }
				);

	std::cout << "NODES PER DEPTH:\n";

	for(unsigned int d=1; d<=m_depth; d++)
	{
		std::vector<Node*> nodes = m_nodes_per_depth[d];
		if (nodes.empty())
			break;

		std::cout << "depth: "<<d;

		for(auto node: nodes)
				std::cout << " " << node;
		std::cout << std::endl;
	}

	std::cout << "POSITION OF NODES IN TREE:\n";
	std::for_each(m_node_at_pos.begin(), m_node_at_pos.end(),[](std::pair<int,Node*> elmt)
				  {
					std::cout << "Position: "<<elmt.first<<" Node: "<<elmt.second << std::endl;
				  }
				);
}

namespace
{
	// precedence of what a node writes: + - 1, * / 2, ^ 3, anything else 4 (never
	// needs parentheses)
	int precedence(Node* node)
	{
		if (node == nullptr || node->type() != NodeType::OPERATOR)
			return 4;

		switch (static_cast<tNode<OPERATOR>*>(node)->data())
		{
		case OPERATOR::ADD:
		case OPERATOR::MINUS:
			return 1;

		case OPERATOR::PROD:
		case OPERATOR::DIV:
			return 2;

		default:
			return 3;
		}
	}

	void write(std::ostream& os, Node* node, int precision);

	void write_operand(std::ostream& os, Node* node, int precision, bool parentheses)
	{
		if (parentheses) os << '(';
		write(os, node, precision);
		if (parentheses) os << ')';
	}

	void write(std::ostream& os, Node* node, int precision)
	{
		if (node == nullptr)
		{
			os << '?';
			return;
		}

		switch (node->type())
		{
		case NodeType::CONSTANT:
		{
			double value = static_cast<tNode<double>*>(node)->data();
			std::ostringstream number;
			number.imbue(std::locale::classic());
			number << std::setprecision(precision) << std::abs(value);

			// the parser would read the sign of an exponent (1e-05) as an operator: group it
			std::string text = number.str();
			if (text.find_first_not_of("0123456789.") != std::string::npos)
				text = "(" + text + ")";

			// a negative constant is a sign before a number, at the start of a group
			if (std::signbit(value))
				os << "(-" << text << ')';
			else
				os << text;
			break;
		}

		case NodeType::VARIABLE:
			os << static_cast<tNode<std::string>*>(node)->data();
			break;

		case NodeType::FUNCTION:
		{
			FUNCTION fn = static_cast<tNode<FUNCTION>*>(node)->data();

			os << '_' << fn2str[fn] << '[';
			write(os, node->left(), precision);

			if (fn == FUNCTION::MIN || fn == FUNCTION::MAX)
			{
				os << ',';
				write(os, node->right(), precision);
			}

			os << ']';
			break;
		}

		case NodeType::OPERATOR:
		{
			int level = precedence(node);
			int left = precedence(node->left());
			int right = precedence(node->right());

			// + - * / are left-associative, so an operand of the same level is grouped
			// on the right (a-(b-c)); ^ is right-associative, so on the left ((a^b)^c)
			write_operand(os, node->left(), precision, left < level || (level == 3 && left == 3));
			os << op2str[static_cast<tNode<OPERATOR>*>(node)->data()];
			write_operand(os, node->right(), precision, right < level || (level < 3 && right == level));
			break;
		}

		default:
			os << '?';
			break;
		}
	}
}

std::string Tree::to_equation(int precision) const
{
	if (m_root == nullptr)
		return "";

	std::ostringstream os;
	os.imbue(std::locale::classic());
	write(os, m_root, precision);

	return os.str();
}

Tree* Tree::clone() const
{
	Tree* copy = new Tree();

	if (m_root != nullptr)
	{
		copy->m_list_variables = m_list_variables;   // build() keeps the values of these
		copy->makeRoot(m_root->clone());
		copy->build();
	}

	return copy;
}

void Tree::print_variables()
{
	std::cout << "List of variables: ";
	for(auto var: m_list_variables)
	{
		std::cout << "\nName: "<<var.first << " Value: "<<var.second;
	}
	std::cout << std::endl;

	return;
}

void Tree::build()
{
	// every index is rebuilt from scratch: the tree may have shrunk since the last build
	m_depth = 0;
	m_nodes.clear();
	m_nodes_per_depth.clear();
	m_node_at_pos.clear();

	if (m_root == nullptr)
	{
		m_list_variables.clear();
		return;
	}

	// compute depth for root AND children
	m_root->s_depth(1);

	m_nodes.reserve(50);
	m_nodes.push_back(m_root);

	// build the list of non-zero nodes
	unsigned pos = 0;
	do
	{
		Node* current = m_nodes[pos++];

		if (current != nullptr)
		{
			m_depth = std::max(m_depth,current->depth());

			Node* left = current->left();
			Node* right = current->right();

			if (left != nullptr) m_nodes.push_back(left);
			if (right != nullptr) m_nodes.push_back(right);
		}
	}
	while(pos < m_nodes.size());

	// variables no longer in the tree are dropped, the others keep their value
	std::map<std::string, double> variables;

	for(auto node: m_nodes)
	{
		// build the list of nodes per depth
		m_nodes_per_depth[node->depth()].push_back(node);

		// build list of variables
		if (node->type() == NodeType::VARIABLE)
		{
			tNode<std::string>* tnode = static_cast<tNode<std::string>*>(node);
			auto it = m_list_variables.find(tnode->data());
			variables.emplace(tnode->data(), (it != m_list_variables.end()) ? it->second : 0.0);
		}
	}

	m_list_variables.swap(variables);

	// build the nodes per position (heap numbering: the children of position p are
	// at 2p+1 and 2p+2). Only existing nodes are visited, so the cost is linear in
	// the number of nodes instead of exponential in the depth.
	std::vector<std::pair<Node*,long long>> positions{{m_root, 0}};
	m_node_at_pos[0] = m_root;

	for(std::size_t i=0; i<positions.size(); i++)
	{
		Node* node = positions[i].first;
		long long p = positions[i].second;

		// nothing below the deepest level, and positions must fit in the int key
		if (node->depth() >= m_depth || 2*p+2 > std::numeric_limits<int>::max())
			continue;

		Node* left = node->left();
		Node* right = node->right();

		m_node_at_pos[2*p+1] = left;
		m_node_at_pos[2*p+2] = right;

		if (left != nullptr) positions.emplace_back(left, 2*p+1);
		if (right != nullptr) positions.emplace_back(right, 2*p+2);
	}

	return;
}

bool Tree::replace(Node* node_to_replace, Node* node)
{
	if (node == nullptr || !this->find_node(node_to_replace))
		return false;

	// a node of this tree in the new subtree would end up with two parents (or be
	// deleted with node_to_replace): graft a clone() instead
	if (this->shares_nodes_with(node))
	{
		std::cerr << "Tree::replace: the new subtree shares nodes with the tree, use a clone(). Nothing done." << std::endl;
		return false;
	}

	if (node_to_replace == m_root)
	{
		node->s_parent(nullptr);
		m_root = node;
	}
	else
	{
		Node* parent = node_to_replace->parent();

		if (parent->left() == node_to_replace)
			parent->s_left(node);
		else
			parent->s_right(node);
	}

	delete node_to_replace;

	this->build();

	return true;
}

void Tree::add_variable(std::string name, double val)
{
	auto it = m_list_variables.find(name);

	if (it != m_list_variables.end())
		m_list_variables[name] = val;
	else
		m_list_variables.emplace(std::pair<std::string,double>(name,val));

	return;
}

double Tree::compute_value(const std::map<std::string,double>& vars)
{
	double val=0.;

	if (m_root == nullptr) // an empty tree has no value
		return std::numeric_limits<double>::quiet_NaN();

	if (m_list_variables.empty()) // Function is constant
		val = this->eval(m_root);
	else
	{
		// Check if all variables are given a value
		for(auto it=m_list_variables.begin(); it!=m_list_variables.end(); ++it)
		{
			auto it1 = vars.find((*it).first);

			if (it1 != vars.end())
			{
				(*it).second = (*it1).second;
			}
			else
			{
				// NaN rather than 0: a value computed without its variables is not a result
				std::cerr << "Parameters list is not complete. Returning NaN." << std::endl;
				return std::numeric_limits<double>::quiet_NaN();
			}
		}

		val = this->eval(m_root);
	}

	return val;
}

double Tree::eval(Node* node)
{
	if (node != nullptr)
	{
		switch (node->type())
		{
		case NodeType::VARIABLE:
		{
			tNode<std::string>* tnode = static_cast<tNode<std::string>*>(node);

			std::string var = tnode->data();

			auto it = m_list_variables.find(var);

			if (it == m_list_variables.end())
			{
				std::cerr << "Variable not found (call build() after renaming one). Returning NaN" << std::endl;
				return std::numeric_limits<double>::quiet_NaN();
			}

			return m_list_variables[var];

			break;
		}
		case NodeType::OPERATOR:
		{
			double val1 = this->eval(node->left());
			double val2 = this->eval(node->right());

			tNode<OPERATOR>* tnode = static_cast<tNode<OPERATOR>*>(node);

			switch (tnode->data())
			{
			case OPERATOR::ADD:
				return val1+val2;

			case OPERATOR::MINUS:
				return val1-val2;

			case OPERATOR::PROD:
				return val1*val2;

			case OPERATOR::DIV:
			{
				// protected division: x/0 is 0 (silently: evolution evaluates many such
				// trees), but an invalid numerator stays invalid
				if (val2 == 0 && std::isfinite(val1))
					return 0.0;
				else if (val2 == 0)
					return val1;
				else
					return val1/val2;

			}

			case OPERATOR::POW:
				// pow(NaN, 0) and pow(1, NaN) are 1: keep the invalid value instead
				if (std::isnan(val1) || std::isnan(val2))
					return std::numeric_limits<double>::quiet_NaN();
				return pow(val1,val2);

			default:
				return 0.0;
			}

			break;
		}

		case NodeType::CONSTANT:
		{
			tNode<double>* tnode = static_cast<tNode<double>*>(node);
			return tnode->data();
			break;
		}

		case NodeType::FUNCTION:
		{
			double val1 = this->eval(node->left());

			tNode<FUNCTION>* tnode = static_cast<tNode<FUNCTION>*>(node);

			switch (tnode->data())
			{
			case FUNCTION::COS:
				return cos(val1);

			case FUNCTION::SIN:
				return sin(val1);

			case FUNCTION::EXP:
				return exp(val1);

			case FUNCTION::LN:
				return log(val1);

			case FUNCTION::LOG:
				return log10(val1);

			// std::max/min would keep or drop a NaN depending on the order of the arguments
			case FUNCTION::MAX:
			{
				double val2 = this->eval(node->right());
				return (std::isnan(val1) || std::isnan(val2)) ? std::numeric_limits<double>::quiet_NaN() : max(val1,val2);
			}

			case FUNCTION::MIN:
			{
				double val2 = this->eval(node->right());
				return (std::isnan(val1) || std::isnan(val2)) ? std::numeric_limits<double>::quiet_NaN() : min(val1,val2);
			}

			case FUNCTION::ROOTSQUARE:
				return sqrt(val1);

			default:
				return 0.0;
			}


			break;
		}

		default:
			break;

		}
	}

	// a missing operand, or a node whose type was never set, has no value
	return std::numeric_limits<double>::quiet_NaN();
}

void Tree::remove_intermediate(Node* node_to_remove, int location)
{
	// location is the child that takes the place of the removed node: left=0 and right=1.
	// The other child is deleted with its subtree.
	if (node_to_remove == nullptr || (location != 0 && location != 1))
		return;

	if (!this->find_node(node_to_remove))
		return;

	Node* kept = (location == 0) ? node_to_remove->left() : node_to_remove->right();
	Node* dropped = (location == 0) ? node_to_remove->right() : node_to_remove->left();

	if (kept == nullptr)
	{
		std::cerr << "Tree::remove_intermediate: the child to keep is empty. Nothing removed." << std::endl;
		return;
	}

	if (node_to_remove == m_root)
	{
		kept->s_parent(nullptr);
		m_root = kept;
	}
	else
	{
		Node* parent = node_to_remove->parent();

		if (parent->which_node(node_to_remove) == 1)
			parent->s_right(kept);
		else
			parent->s_left(kept);
	}

	// detach the children so that deleting the node does not free the kept subtree
	node_to_remove->s_children(nullptr,nullptr);
	delete node_to_remove;
	delete dropped;

	this->build();

	return;
}
