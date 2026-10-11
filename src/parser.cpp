#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

void CParser::print_expr()
{
	for(auto it=m_list_expr.begin(); it!=m_list_expr.end(); it++)
		std::cout << it->first << " " << it->second << std::endl;

	return;
}

void CParser::print_func()
{
	for(auto it=m_list_func.begin(); it!=m_list_func.end(); it++)
		std::cout << it->first << " " << fn2str[(it->second).first] << " " << (it->second).second <<std::endl;

	return;
}

void CParser::build_tree_from_equation(Tree* tree, std::string equation)
{
	/*
	 * First we transform the equation to get the most simple form
	 * which only contains + / - operators
	 */
	tree->clear(1);

	Node* root = this->parse(equation);

	if (root == nullptr)
	{
		std::cerr << "Unable to parse the equation \"" << equation << "\": the tree is left empty." << std::endl;
		return;
	}

	tree->makeRoot(root);
	tree->build();

	return;
}

bool CParser::parse_function(std::string& equation)
{
	// Replace each call _name[argument] by a placeholder #funcN#. The argument may not
	// contain brackets, so the innermost calls are replaced first: nested calls are
	// resolved from the inside out and two calls are never merged into one.
	std::regex function("_([a-zA-Z]+)\\[([^\\[\\]]*)\\]");
	std::smatch sm;
	std::ostringstream os;

	while (std::regex_search(equation,sm,function))
	{
		auto fn = str2fn.find(sm.str(1));

		if (fn == str2fn.end())
		{
			std::cerr << "Unknown function \"" << sm.str(1) << "\"" << std::endl;
			return false;
		}

		os.str("");
		os << "#func" << m_list_func.size() << "#";
		m_list_func.emplace(os.str(), std::make_pair(fn->second, sm.str(2)));

		equation.replace(sm.position(0),sm.length(0),os.str());
	}

	// what is left is a call that is not properly opened or closed
	if (equation.find_first_of("_[]") != std::string::npos)
	{
		std::cerr << "Unbalanced brackets in \"" << equation << "\"" << std::endl;
		return false;
	}

	return true;
}

bool CParser::parse_factor_between_parenthesis(std::string& equation)
{
	// Replace each group (...) by a placeholder #exprN#, innermost groups first
	std::regex parenthesis("\\(([^\\(\\)]*)\\)");
	std::smatch sm;
	std::ostringstream os;

	while (std::regex_search(equation,sm,parenthesis))
	{
		os.str("");
		os << "#expr" << m_list_expr.size() << "#";
		m_list_expr.emplace(os.str(),sm.str(1));

		equation.replace(sm.position(0),sm.length(0),os.str());
	}

	// a parenthesis left alone is not balanced
	if (equation.find_first_of("()") != std::string::npos)
	{
		std::cerr << "Unbalanced parentheses in \"" << equation << "\"" << std::endl;
		return false;
	}

	return true;
}

bool CParser::parse_operator_level(std::string& equation, const std::regex& level, bool unary, Node*& parent)
{
	// Folds every "left op right" of one precedence level from left to right, so the
	// operators are left-associative: a-b-c is (a-b)-c. 'parent' holds the tree built
	// so far. A leading operator is accepted only if 'unary' is set: -x is read as 0-x.
	std::smatch sm;

	while (std::regex_search(equation,sm,level))
	{
		if (VERBOSE)
		{
			std::cout << "--[" << sm.str(0) << "] at position "<<sm.position(0)<<"\n";
			std::cout << "--[" << sm.str(1) << "] at position "<<sm.position(1)<<"\n";
			std::cout << "--[" << sm.str(2) << "] at position "<<sm.position(2)<<"\n";
			std::cout << "--[" << sm.str(3) << "] at position "<<sm.position(3)<<std::endl;
		}

		if (sm.position(2) > 0 && parent != nullptr)
		{
			// an operator right after another one, e.g. the "+-" of "x+y+-z"
			std::cerr << "Unexpected \"" << sm.str(1) << "\" in \"" << equation << "\"" << std::endl;
			delete parent;
			parent = nullptr;
			return false;
		}

		Node* left = nullptr;
		bool sign = (sm.position(2) == 0 && parent == nullptr && unary);

		if (sm.position(2) > 0)
			left = this->parse_expression(sm.str(1));
		else if (parent != nullptr)
			left = parent;
		else if (unary)
		{
			left = new tNode<double>(0.0);
			left->s_type(NodeType::CONSTANT);
		}

		Node* right = this->parse_expression(sm.str(3));

		if (left == nullptr || right == nullptr)
		{
			std::cerr << "Missing operand around \"" << sm.str(2) << "\" in \"" << equation << "\"" << std::endl;
			delete left;
			delete right;
			parent = nullptr;
			return false;
		}

		if (sign && right->type() == NodeType::CONSTANT)
		{
			// a sign before a number belongs to it: -2.5 is the constant -2.5, not 0-2.5
			tNode<double>* number = static_cast<tNode<double>*>(right);

			if (sm.str(2) == "-")
				number->s_data(-number->data());

			delete left;
			parent = right;
		}
		else
		{
			parent = new tNode<OPERATOR>(str2op[sm.str(2)]);
			parent->s_type(NodeType::OPERATOR);
			parent->s_children(left,right);
		}

		equation = sm.suffix().str();
	}

	return true;
}

Node* CParser::parse_function_call(const std::pair<FUNCTION, std::string>& call)
{
	FUNCTION fn = call.first;
	const std::string& argument = call.second;

	// min and max take two arguments separated by a comma: _min[a,b]
	std::size_t comma = std::string::npos;
	int depth = 0;

	for (std::size_t i=0; i<argument.size() && comma == std::string::npos; i++)
	{
		if (argument[i] == '(') depth++;
		else if (argument[i] == ')') depth--;
		else if (argument[i] == ',' && depth == 0) comma = i;
	}

	Node* left = nullptr;
	Node* right = nullptr;

	if (fn == FUNCTION::MIN || fn == FUNCTION::MAX)
	{
		if (comma == std::string::npos)
		{
			std::cerr << fn2str[fn] << " needs two arguments: _" << fn2str[fn] << "[a,b]" << std::endl;
			return nullptr;
		}

		left = this->parse_expression(argument.substr(0,comma));
		right = this->parse_expression(argument.substr(comma+1));

		if (left == nullptr || right == nullptr)
		{
			delete left;
			delete right;
			return nullptr;
		}
	}
	else
	{
		left = this->parse_expression(argument);

		if (left == nullptr)
			return nullptr;
	}

	Node* node = new tNode<FUNCTION>(fn);
	node->s_type(NodeType::FUNCTION);
	node->s_children(left,right);

	return node;
}

Node* CParser::parse(std::string equation)
{
	// '#' delimits the placeholders of the groups and function calls (#expr0#, #func0#):
	// the equation itself cannot contain one
	if (equation.find('#') != std::string::npos)
	{
		std::cerr << "Unexpected '#' in \"" << equation << "\"" << std::endl;
		return nullptr;
	}

	// spaces may separate tokens but not split one: "x y" is not the variable xy, and
	// "1e - 3" is not the number 1e-3
	if (std::regex_search(equation, std::regex("[A-Za-z0-9._]\\s+[A-Za-z0-9._]|[0-9.][eE]\\s*[+-]\\s+[0-9]|[0-9.][eE]\\s+[+-]")))
	{
		std::cerr << "Unexpected space inside or between two terms in \"" << equation << "\"" << std::endl;
		return nullptr;
	}

	// the ASCII spaces only: std::isspace follows the C locale, which QApplication
	// changes (a non-breaking space byte could then glue two tokens)
	equation.erase(std::remove_if(equation.begin(), equation.end(),
								  [](char c){ return std::strchr(" \t\n\v\f\r", c) != nullptr && c != '\0'; }),
				   equation.end());

	// the placeholders of a previous equation are not needed anymore
	m_list_expr.clear();
	m_list_func.clear();

	return this->parse_expression(equation);
}

Node* CParser::parse_expression(std::string equation)
{
	if (equation.empty())
	{
		std::cerr << "Empty expression, e.g. \"()\" or \"_exp[]\"" << std::endl;
		return nullptr;
	}

	if (VERBOSE)
	{
		std::cout << "-----------------------------------------\n";
		std::cout << "             NOUVELLE EQUATION           \n";
		std::cout << "-----------------------------------------\n";
		std::cout << equation <<std::endl;
		std::cout << "-----------------------------------------\n";
	}

	if (is_number(equation))
	{
		Node* node = new tNode<double>(to_number(equation));
		node->s_type(NodeType::CONSTANT);
		return node;
	}
	else if (is_variable(equation))
	{
		Node* node = new tNode<std::string>(equation);
		node->s_type(NodeType::VARIABLE);
		return node;
	}

	if (!this->parse_function(equation) || !this->parse_factor_between_parenthesis(equation))
		return nullptr;

	if (VERBOSE)
	{
		std::cout << "1ere EQUATION: " << equation << std::endl;
	}

	// + and - have the lowest precedence, then * and /
	Node* parent = nullptr;

	if (!this->parse_operator_level(equation, std::regex("(.*?)([\\+\\-])([^\\+\\-]+)"), true, parent) ||
		!this->parse_operator_level(equation, std::regex("(.*?)([\\*/])([^\\*/]+)"), false, parent))
		return nullptr;

	if (parent != nullptr)
	{
		// only a dangling operator can be left, e.g. "x+1+"
		if (!equation.empty())
		{
			std::cerr << "Unexpected \"" << equation << "\" at the end of an expression" << std::endl;
			delete parent;
			return nullptr;
		}

		return parent;
	}

	// ^ is right-associative: a^b^c is a^(b^c). No other operator is left at this
	// level, so every ^ separates two operands: each piece is parsed once and the
	// powers are folded from the right
	if (equation.find('^') != std::string::npos)
	{
		std::vector<std::string> pieces;
		std::size_t start = 0, hat;

		while ((hat = equation.find('^', start)) != std::string::npos)
		{
			pieces.push_back(equation.substr(start, hat-start));
			start = hat+1;
		}

		pieces.push_back(equation.substr(start));

		Node* power = nullptr;   // the part already folded, on the right

		for (auto piece = pieces.rbegin(); piece != pieces.rend(); ++piece)
		{
			Node* operand = piece->empty() ? nullptr : this->parse_expression(*piece);

			if (operand == nullptr)
			{
				std::cerr << "Missing operand around \"^\" in \"" << equation << "\"" << std::endl;
				delete power;
				return nullptr;
			}

			if (power == nullptr)
				power = operand;
			else
			{
				parent = new tNode<OPERATOR>(OPERATOR::POW);
				parent->s_type(NodeType::OPERATOR);
				parent->s_children(operand,power);
				power = parent;
			}
		}

		return power;
	}

	// what is left must be a single placeholder: a function call or a group between parenthesis
	auto func = m_list_func.find(equation);

	if (func != m_list_func.end())
		return this->parse_function_call(func->second);

	auto expr = m_list_expr.find(equation);

	if (expr != m_list_expr.end())
		return this->parse_expression(expr->second);

	std::cerr << "Unable to parse \"" << equation << "\"" << std::endl;

	return nullptr;
}
