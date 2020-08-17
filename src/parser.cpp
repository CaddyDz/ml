#include "parser.hpp"

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
	tree->makeRoot(root);
	tree->build();

	return;
}

void CParser::parse_function(std::string& equation)
{
	// First we group parenthesis
	std::regex function("_(.*?)\\[(.*)\\]");
	std::smatch sm;
	std::ostringstream os;
	std::string test;

	while (equation.find("_") != std::string::npos)
	{
		test = equation;

		while (std::regex_search(test,sm,function))
		{
			os << "func" << m_list_func.size();
			int pos = equation.find(sm.str(0));
			equation.replace(pos,sm.str(0).length(),os.str());

			m_list_func.emplace(os.str(), std::make_pair(str2fn[sm.str(1)],sm.str(2)));
			os.str("");

			test = sm.suffix().str();
		}
	}

	return;
}

void CParser::parse_factor_between_parenthesis(std::string& equation)
{
	// First we group parenthesis
	std::regex parenthesis("\\(([^\\(]*?)\\)");
	std::smatch sm;
	std::ostringstream os;
	std::string test;

	while (equation.find("(") != std::string::npos)
	{
		test = equation;

		while (std::regex_search(test,sm,parenthesis))
		{
			os << "expr" << m_list_expr.size();
			int pos = equation.find(sm.str(0));
			equation.replace(pos,sm.str(0).length(),os.str());

			m_list_expr.emplace(os.str(),sm.str(1));
			os.str("");

			test = sm.suffix().str();
		}
	}

	return;
}

Node* CParser::parse(std::string equation)
{
	if (equation.empty())
		return nullptr;

	if (VERBOSE)
	{
		std::cout << "-----------------------------------------\n";
		std::cout << "             NOUVELLE EQUATION           \n";
		std::cout << "-----------------------------------------\n";
		std::cout << equation <<std::endl;
		std::cout << "-----------------------------------------\n";
	}

	std::smatch sm;

	if (is_number(equation))
	{
		Node* node = new tNode<double>(std::stod(equation));
		node->s_type(NodeType::CONSTANT);
		return node;
	}
	else if (is_variable(equation))
	{
		Node* node = new tNode<std::string>(equation);
		node->s_type(NodeType::VARIABLE);
		return node;
	}
	else
	{
		this->parse_function(equation);
		this->parse_factor_between_parenthesis(equation);

		Node* parent, *left, *right;


		if (VERBOSE)
		{
			std::cout << "1ere EQUATION: " << equation << std::endl;
		}

		std::regex prec2("(.*?)([\\+\\-])([^\\+\\-]+)");

		while (std::regex_search(equation,sm,prec2))
		{
			if (VERBOSE)
			{
				std::cout << "--[" << sm.str(0) << "] at position "<<sm.position(0)<<"\n";
				std::cout << "--[" << sm.str(1) << "] at position "<<sm.position(1)<<"\n";
				std::cout << "--[" << sm.str(2) << "] at position "<<sm.position(2)<<"\n";
				std::cout << "--[" << sm.str(3) << "] at position "<<sm.position(3)<<std::endl;
			}

			left =  (sm.position(2) > 0) ? this->parse(sm.str(1)) : parent;
			parent = new tNode<OPERATOR>(str2op[sm.str(2)]);
			parent->s_type(NodeType::OPERATOR);
			right = this->parse(sm.str(3));

			parent->s_children(left,right);

			equation = sm.suffix().str();
		}

		if (VERBOSE)
		{
			std::cout << "2eme EQUATION: " << equation << std::endl;
		}

		std::regex prec3("(.*?)(\\*)([^\\*]+)");

		while (std::regex_search(equation,sm,prec3))
		{
			if (VERBOSE)
			{
				std::cout << "--[" << sm.str(0) << "] at position "<<sm.position(0)<<"\n";
				std::cout << "--[" << sm.str(1) << "] at position "<<sm.position(1)<<"\n";
				std::cout << "--[" << sm.str(2) << "] at position "<<sm.position(2)<<"\n";
				std::cout << "--[" << sm.str(3) << "] at position "<<sm.position(3)<<std::endl;
			}

			left =  (sm.position(2) > 0) ? this->parse(sm.str(1)) : parent;
			parent = new tNode<OPERATOR>(str2op[sm.str(2)]);
			parent->s_type(NodeType::OPERATOR);
			right = this->parse(sm.str(3));

			parent->s_children(left,right);

			equation = sm.suffix().str();
		}

		if (VERBOSE)
		{
			std::cout << "3eme EQUATION: " << equation << std::endl;
		}

		std::regex prec4("(.*?)(/)([^/]+)");

		while (std::regex_search(equation,sm,prec4))
		{
			if (VERBOSE)
			{
				std::cout << "--[" << sm.str(0) << "] at position "<<sm.position(0)<<"\n";
				std::cout << "--[" << sm.str(1) << "] at position "<<sm.position(1)<<"\n";
				std::cout << "--[" << sm.str(2) << "] at position "<<sm.position(2)<<"\n";
				std::cout << "--[" << sm.str(3) << "] at position "<<sm.position(3)<<std::endl;
			}

			left =  (sm.position(2) > 0) ? this->parse(sm.str(1)) : parent;
			parent = new tNode<OPERATOR>(str2op[sm.str(2)]);
			parent->s_type(NodeType::OPERATOR);
			right = this->parse(sm.str(3));

			parent->s_children(left,right);

			equation = sm.suffix().str();
		}

		if (VERBOSE)
		{
			std::cout << "4eme EQUATION: " << equation << std::endl;
		}

		std::regex prec5("(.*?)(\\^)([^\\^]+)");

		while (std::regex_search(equation,sm,prec5))
		{
			if (VERBOSE)
			{
				std::cout << "--[" << sm.str(0) << "] at position "<<sm.position(0)<<"\n";
				std::cout << "--[" << sm.str(1) << "] at position "<<sm.position(1)<<"\n";
				std::cout << "--[" << sm.str(2) << "] at position "<<sm.position(2)<<"\n";
				std::cout << "--[" << sm.str(3) << "] at position "<<sm.position(3)<<std::endl;
			}

			left =  (sm.position(2) > 0) ? this->parse(sm.str(1)) : parent;
			parent = new tNode<OPERATOR>(str2op[sm.str(2)]);
			parent->s_type(NodeType::OPERATOR);
			right = this->parse(sm.str(3));

			parent->s_children(left,right);

			equation = sm.suffix().str();
		}

		if (VERBOSE)
		{
			std::cout << "5eme EQUATION: " << equation <<std::endl;
		}

		std::regex is_a_function("^func.*");

		if(std::regex_search(equation,sm,is_a_function))
		{
			parent = new tNode<FUNCTION>(m_list_func[sm.str(0)].first);
			parent->s_type(NodeType::FUNCTION);
			left = this->parse(m_list_func[sm.str(0)].second);

			parent->s_children(left,nullptr);
		}

		if (VERBOSE)
		{
			std::cout << "6eme EQUATION:" << equation <<std::endl;
		}

		std::regex is_an_expression("^expr.*");

		if (std::regex_search(equation,sm,is_an_expression))
		{
			parent = this->parse(m_list_expr[sm.str(0)]);
		}

		return parent;
	}

	return nullptr;
}
