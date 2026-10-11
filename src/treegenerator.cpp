#include "treegenerator.hpp"

#include <cmath>
#include <ctime>

CTreeGenerator::CTreeGenerator(const Rules& rules)
{
	m_rules = rules;

	// the root is always an operator, so a tree has at least two levels
	if (m_rules.max_depth < 2)
	{
		std::cerr << "Rules: max_depth must be at least 2, using 2" << std::endl;
		m_rules.max_depth = 2;
	}

	if (m_rules.max_polynomial_order < 1)
	{
		std::cerr << "Rules: max_polynomial_order must be at least 1, using 1" << std::endl;
		m_rules.max_polynomial_order = 1;
	}

	// the constants are drawn uniformly: the range must be finite (NaN included)
	if (!std::isfinite(m_rules.max_number - m_rules.min_number))
	{
		std::cerr << "Rules: the range of the constants must be finite, using [-1, 1]" << std::endl;
		m_rules.min_number = -1;
		m_rules.max_number = 1;
	}

	if (m_rules.min_number > m_rules.max_number)
		std::swap(m_rules.min_number, m_rules.max_number);

	// one engine per generator: std::random_device can be deterministic (MinGW before
	// GCC 9.2), so the seed also mixes the time and a count of the generators, which
	// keeps two generators created within the same second apart
	static unsigned int created = 0;
	std::random_device rd;
	m_engine.seed(rd() ^ static_cast<unsigned int>(std::time(nullptr)) ^ (++created * 2654435761u));
}

double CTreeGenerator::random_constant() const
{
	std::uniform_real_distribution<double> distribution(m_rules.min_number, m_rules.max_number);

	return distribution(m_engine);
}

std::size_t CTreeGenerator::random_index(std::size_t count) const
{
	// a uniform index in [0, count), count > 0
	std::uniform_int_distribution<std::size_t> distribution(0, count-1);

	return distribution(m_engine);
}

void CTreeGenerator::generate_node(Node*& node, unsigned int depth) const
{
	Node* left, *right;

	NodeType type;

	if (depth == 1)
		type = NodeType::OPERATOR;
	else if (depth >= m_rules.max_depth)
		type = static_cast<NodeType>(this->random_index(2));
	else
		type = static_cast<NodeType>(this->random_index(4));

	// without any variable a leaf can only be a constant
	if (type == NodeType::VARIABLE && m_rules.m_list_variables.empty())
		type = NodeType::CONSTANT;

	switch(type)
	{
	case NodeType::CONSTANT:
	{
		double val = this->random_constant();
		node = new tNode<double>(val);
		node->s_type(NodeType::CONSTANT);
		node->s_depth(depth);
		break;
	}

	case NodeType::VARIABLE:
	{
		std::size_t ivar = this->random_index(m_rules.m_list_variables.size());

		node = new tNode<std::string>(m_rules.m_list_variables[ivar]);
		node->s_type(NodeType::VARIABLE);
		node->s_depth(depth);

		break;
	}

	case NodeType::FUNCTION:
	{
		FUNCTION fn = static_cast<FUNCTION>(this->random_index(8));

		node = new tNode<FUNCTION>(fn);
		node->s_type(NodeType::FUNCTION);
		node->s_depth(depth++);

		if (fn == FUNCTION::MIN || fn == FUNCTION::MAX)
		{
			this->generate_node(left,depth);
			this->generate_node(right,depth);
			node->s_children(left,right);
		}
		else
		{
			this->generate_node(left,depth);
			node->s_left(left);
		}

		break;
	}

	case NodeType::OPERATOR:
	{
		OPERATOR op = static_cast<OPERATOR>(this->random_index(5));

		node = new tNode<OPERATOR>(op);
		node->s_type(NodeType::OPERATOR);
		node->s_depth(depth++);

		this->generate_node(left,depth);
		this->generate_node(right,depth);
		node->s_children(left,right);

		break;
	}

	default:
		break;
	}

	return;
}

Tree* CTreeGenerator::generate_tree() const
{
	Tree* generated_tree = new Tree();

	Node* root;
	this->generate_node(root,1);

	generated_tree->makeRoot(root);
	generated_tree->build();

	// check the tree afterwards
	this->check_rules(generated_tree);

	return generated_tree;
}

bool CTreeGenerator::check_rules(Tree* tree) const
{
	if (!tree)
	{
		std::cerr<<"Tree is empty"<<std::endl;
		return false;
	}
	else if( tree->depth() > m_rules.max_depth)
	{
		std::cerr<<"Tree max depth exceeded"<<std::endl;
		return false;
	}

	std::vector<Node*> nodes = tree->nodes();

	// Remove obvious subtrees, for instance */0, *^infinity, *^0,
	// TODO exp(ln(.)), sqrt(x^2), sqrt(x)^2

	std::for_each(nodes.begin(), nodes.end(), [this, &tree](Node* node)
	{
		if ((node != nullptr) && tree->find_node(node)) // must ensure that node has not been destroyed by a previous check
		{
			if (node->type() == NodeType::OPERATOR)
			{
				tNode<OPERATOR>* op = static_cast<tNode<OPERATOR>*>(node);
				Node* right = node->right();

				if (right == nullptr)
					return;

				NodeType type = right->type();

				// Check for max polynomial order
				if (op->data() == OPERATOR::POW)
				{
					/*
					 * Two possible cases :
					 *    * right is a number, we must ensure this number is smaller than max_polynomial_order
					 *    * right is anything else (exponent can be very high, implying an overflow), we replace it by a number which follows the previous rules
					 */

					if (type == NodeType::CONSTANT)
					{
						// the exponent must be a whole number between 1 and max_polynomial_order:
						// a fractional power of a negative number is NaN, a negative power of 0 infinite
						tNode<double>* exponent = static_cast<tNode<double>*>(right);
						double order = std::round(exponent->data());

						if (!(order >= 1 && order <= this->m_rules.max_polynomial_order)) // NaN included
							order = this->m_rules.max_polynomial_order;

						exponent->s_data(order);
					}
					else
					{
						double exp = this->random_index(this->m_rules.max_polynomial_order)+1;
						tNode<double>* exponent = new tNode<double>(exp);
						exponent->s_type(NodeType::CONSTANT);

						// on mute le noeud
						if (!tree->replace(right,exponent))
							delete exponent;

					}
				}
				else if (op->data() == OPERATOR::DIV)
				{
					Node* right = node->right();
					NodeType type = right->type();

					if (type == NodeType::CONSTANT)
					{
						tNode<double>* div = static_cast<tNode<double>*>(right);

						if (div->data() == 0.0)
						{
							double val = this->random_constant();

							// a range reduced to {0} would draw 0 again
							div->s_data(val != 0.0 ? val : 1.0);
						}
					}
				}
			}
		}
	}
	);

	return true;
}

Node* CTreeGenerator::generate_subtree(unsigned int depth) const
{
	Node* node = nullptr;
	this->generate_node(node, std::max(depth, 1u));

	return node;
}

Node* CTreeGenerator::pick_node(const Tree& tree) const
{
	// Koza's choice: an internal node 90% of the time, so that crossover and
	// mutation do not mostly swap leaves
	std::vector<Node*> internal, leaves;

	for (Node* node : tree.nodes())
		(node->left() != nullptr || node->right() != nullptr ? internal : leaves).push_back(node);

	std::uniform_real_distribution<double> uniform(0.0, 1.0);
	const std::vector<Node*>& pool = (!internal.empty() && (leaves.empty() || uniform(m_engine) < 0.9)) ? internal : leaves;

	return pool[this->random_index(pool.size())];
}

Tree* CTreeGenerator::crossover(const Tree& mother, const Tree& father) const
{
	if (mother.isEmpty() || father.isEmpty())
		return mother.clone();

	for (int attempt = 0; attempt < 10; ++attempt)
	{
		Tree* child = mother.clone();
		Node* graft = this->pick_node(father)->clone();

		if (!child->replace(this->pick_node(*child), graft))
		{
			delete graft;
			delete child;
			continue;
		}

		if (child->depth() <= m_rules.max_depth)
		{
			this->check_rules(child);
			return child;
		}

		delete child;
	}

	return mother.clone();
}

void CTreeGenerator::point_mutation(Node* node) const
{
	switch (node->type())
	{
	case NodeType::OPERATOR:
	{
		// one of the four other binary operators
		tNode<OPERATOR>* op = static_cast<tNode<OPERATOR>*>(node);
		std::size_t current = static_cast<std::size_t>(op->data());
		op->s_data(static_cast<OPERATOR>((current + 1 + this->random_index(4)) % 5));
		break;
	}

	case NodeType::FUNCTION:
	{
		// a function of the same arity: min <-> max, or another one-argument function
		tNode<FUNCTION>* fn = static_cast<tNode<FUNCTION>*>(node);
		const FUNCTION unary[] = {FUNCTION::EXP, FUNCTION::COS, FUNCTION::SIN, FUNCTION::LOG, FUNCTION::LN, FUNCTION::ROOTSQUARE};

		if (fn->data() == FUNCTION::MIN)
			fn->s_data(FUNCTION::MAX);
		else if (fn->data() == FUNCTION::MAX)
			fn->s_data(FUNCTION::MIN);
		else
		{
			FUNCTION other = fn->data();
			while (other == fn->data())
				other = unary[this->random_index(6)];
			fn->s_data(other);
		}
		break;
	}

	case NodeType::CONSTANT:
	{
		// a small step, relative to the size of the constant: tunes the coefficients
		tNode<double>* constant = static_cast<tNode<double>*>(node);
		double value = constant->data();
		std::normal_distribution<double> step(0.0, 0.1 * std::max(1.0, std::abs(value)));
		constant->s_data(value + step(m_engine));
		break;
	}

	case NodeType::VARIABLE:
	{
		const std::vector<std::string>& variables = m_rules.m_list_variables;
		tNode<std::string>* variable = static_cast<tNode<std::string>*>(node);

		if (variables.size() > 1)
		{
			std::string other = variable->data();
			while (other == variable->data())
				other = variables[this->random_index(variables.size())];
			variable->s_data(other);
		}
		break;
	}

	default:
		break;
	}
}

Tree* CTreeGenerator::mutate(const Tree& parent) const
{
	if (parent.isEmpty())
		return parent.clone();

	std::uniform_real_distribution<double> uniform(0.0, 1.0);

	for (int attempt = 0; attempt < 10; ++attempt)
	{
		Tree* child = parent.clone();
		Node* target = this->pick_node(*child);

		if (uniform(m_engine) < 0.5)
		{
			// subtree mutation: the new subtree's leaves stop at max_depth
			Node* subtree = this->generate_subtree(target->depth());

			if (!child->replace(target, subtree))
			{
				delete subtree;
				delete child;
				continue;
			}
		}
		else
		{
			this->point_mutation(target);
			child->build();   // a renamed variable must enter the variable list
		}

		if (child->depth() <= m_rules.max_depth)
		{
			this->check_rules(child);   // e.g. a new ^ needs a valid exponent
			return child;
		}

		delete child;
	}

	return parent.clone();
}
