#include "tree.hpp"

Tree::~Tree()
{
	if (m_root) delete m_root;
}

void Tree::clear(bool force_delete)
{
	if (force_delete)
		if (m_root) delete m_root;

	m_root = nullptr;
	m_nodes.clear();
	m_nodes_per_depth.clear();
	m_node_at_pos.clear();
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
	auto it = m_nodes_per_depth.find(dp);

	if (it != m_nodes_per_depth.end())
		return m_nodes_per_depth[dp];
	else
		return std::vector<Node*>();

}

Node* Tree::find_node_from_val(std::string val)
{
	if (is_number(val))
	{
		double vald = std::stod(val);

		auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
							   [&vald](Node* node)
								{
									if (node->type() == NodeType::CONSTANT)
									{
										tNode<double>* tnode = static_cast<tNode<double>*>(node);
										return (tnode->data() == vald);
									}
								}
							   );

		if (it != m_nodes.end())
			return *it;
		else
			return nullptr;
	}
	else if (is_operator(val))
	{
		OPERATOR vald = str2op[val];

		auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
							   [&vald](Node* node)
								{
									if (node->type() == NodeType::OPERATOR)
									{
										tNode<OPERATOR>* tnode = static_cast<tNode<OPERATOR>*>(node);
										return (tnode->data() == vald);
									}
								}
							   );

		if (it != m_nodes.end())
			return *it;
		else
			return nullptr;
	}
	else if (is_variable(val))
	{

		auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
							   [&val](Node* node)
								{
									if (node->type() == NodeType::VARIABLE)
									{
										tNode<std::string>* tnode = static_cast<tNode<std::string>*>(node);
										return (tnode->data() == val);
									}
								}
							   );

		if (it != m_nodes.end())
			return *it;
		else
			return nullptr;
	}
	else if (is_func(val))
	{
		FUNCTION vald = str2fn[val];

		auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
							   [&vald](Node* node)
								{
									if (node->type() == NodeType::FUNCTION)
									{
										tNode<FUNCTION>* tnode = static_cast<tNode<FUNCTION>*>(node);
										return (tnode->data() == vald);
									}
								}
							   );

		if (it != m_nodes.end())
			return *it;
		else
			return nullptr;
	}

	return nullptr;



}

Node* Tree::find_node_from_val_at_depth(std::string val, unsigned int dp)
{
	// identification of the value type
	NodeType type;
	if (is_variable(val))
		type = NodeType::VARIABLE;
	else if (is_number(val))
		type = NodeType::CONSTANT;
	else if (is_func(val))
		type = NodeType::FUNCTION;
	else if (is_operator(val))
		type = NodeType::OPERATOR;
	else
	{
		std::cerr << "Type of the value not recognized. Exiting function find_node_from_val_at_depth" << std::endl;
		return nullptr;
	}

	auto it = m_nodes_per_depth.find(dp);

	if (it != m_nodes_per_depth.end())
	{
		std::vector<Node*> nodes_at_depth = it->second;

		for(auto node: nodes_at_depth)
		{
			if (node->type() == type)
			{
				switch (type)
				{
				case NodeType::CONSTANT:
				{
					tNode<double>* tnb = static_cast<tNode<double>*>(node);
					if (tnb->data() == std::stod(val)) return node;
					break;
				}

				case NodeType::OPERATOR:
				{
					tNode<OPERATOR>* top = static_cast<tNode<OPERATOR>*>(node);
					if (top->data() == str2op[val]) return node;
					break;
				}

				case NodeType::FUNCTION:
				{
					tNode<FUNCTION>* tfunc = static_cast<tNode<FUNCTION>*>(node);
					if (tfunc->data() == str2fn[val]) return node;
					break;
				}

				case NodeType::VARIABLE:
				{
					tNode<std::string>* tvar = static_cast<tNode<std::string>*>(node);
					if (tvar->data() == val) return node;
					break;
				}

				default:
					std::cerr << "Type of the node not recognized. Exiting function find_node_from_val_at_depth" << std::endl;
					return nullptr;
					break;
				}
			}
		}
	}
	return nullptr;
}

void Tree::makeRoot(Node* root)
{
	if (m_root != NULL)
	{
		std::cerr << "WARNING: Change of Root node." << std::endl;
		return;
	}

	m_root = root;
	m_root->s_depth(1);
}

void Tree::expand_from_root(Node* node)
{
	if (node != nullptr)
	{
		node->s_left(m_root);
		node->s_right(nullptr);

		m_root = node;
		m_root->s_depth(1);

		// rebuild the tree
		this->build();
	}
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
	// compute depth for root AND children
	m_root->s_depth(1);

	m_nodes.clear();
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

	m_nodes_per_depth.clear();

	for(auto node: m_nodes)
	{
		// build the list of nodes per depth
		m_nodes_per_depth[node->depth()].push_back(node);

		// build list of variables
		if (node->type() == NodeType::VARIABLE)
		{
			tNode<std::string>* tnode = static_cast<tNode<std::string>*>(node);
			m_list_variables.emplace(std::pair<std::string,double>(tnode->data(),0));
		}
	}

	// build the nodes per position
	m_node_at_pos.clear();

	unsigned int max_nodes_nb = pow(2,m_depth-1);
	m_node_at_pos[0] = m_root;
	for(pos=0; pos<max_nodes_nb; pos++)
	{
		auto it = m_node_at_pos.find(pos);

		if (it != m_node_at_pos.end())
		{
			Node* node = it->second;

			if (node != nullptr)
			{
			  Node* left = node->left();
			  Node* right = node->right();

			  m_node_at_pos[2*pos+1] = left;
			  m_node_at_pos[2*pos+2] = right;
			}
		}
	}while(pos < max_nodes_nb)

	return;
}

void Tree::replace(Node* node_to_replace, Node* node)
{
	if (node)
	{
		if (this->find_node(node_to_replace))
		{
			Node* parent = node_to_replace->parent();

			if (parent->left() == node_to_replace)
				parent->s_left(node);
			else
				parent->s_right(node);

			node->s_parent(parent);
			delete node_to_replace;

			this->build();
		}
	}
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
				std::cerr << "Parameters list is not complete. Returning 0." << std::endl;
				val = 0.0;
				return 0.0;
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
				std::cerr << "Variable not found. Returning 0.0" << std::endl;
				return 0.0;
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
				if (val2 == 0)
				{
					std::cerr << "Division by 0. Returning 0." << std::endl;
					return 0.0;
				}
				else
					return val1/val2;

			}

			case OPERATOR::POW:
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
				return log2(val1);

			case FUNCTION::LOG:
				return log(val1);

			case FUNCTION::MAX:
			{
				double val2 = this->eval(node->right());
				return max(val1,val2);
			}

			case FUNCTION::MIN:
			{
				double val2 = this->eval(node->right());
				return min(val1,val2);
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

	return 0.0;
}

void Tree::remove_intermediate(Node* node_to_remove, int location)
{
	std::cout << "On entre ici"<<std::endl;

	if (node_to_remove == nullptr)
		return;

	// location stands for left=0 and right=1;

	// Remove parent and copy left instead
	auto it = std::find(m_nodes.begin(), m_nodes.end(),node_to_remove);

	if (it != m_nodes.end())
	{
		Node* left = (*it)->left();
		Node* right = (*it)->right();
		Node* parent = (*it)->parent();

		// Remove right node
		if (location == 0)
		{
			if (right != nullptr) delete right;

			left->s_parent(parent);
			if (parent->which_node(node_to_remove))
				parent->s_right(left);
			else
				parent->s_left(left);

			node_to_remove->s_children(nullptr,nullptr);
			delete node_to_remove;
		}
		else if (location == 1)
		{
			if (left != nullptr) delete left;

			right->s_parent(parent);
			if (parent->which_node(node_to_remove))
				parent->s_right(right);
			else
				parent->s_left(right);

			node_to_remove->s_children(nullptr,nullptr);
			delete node_to_remove;


		}

		this->build();
	}

	return;
}
