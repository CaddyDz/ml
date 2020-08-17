#ifndef PARSER_HPP
#define PARSER_HPP

#define VERBOSE 0

#include <string>
#include <vector>
#include <stack>
#include <regex>
#include <list>
#include <iomanip>
#include <iostream>
#include <strstream>
#include <map>
#include "tree.hpp"

class CParser
{
private:
	std::map<std::string, std::string> m_list_expr;
	std::map<std::string, std::pair<FUNCTION, std::string> > m_list_func;

public:
	CParser(){}
	CParser(const CParser&) = delete;
	~CParser(){}

	void print_expr();
	void print_func();

	Node* parse(std::string);
	bool parse_simple_operation(std::string, Node*);
	void parse_factor_between_parenthesis(std::string&);
	void parse_function(std::string&);
	void build_tree_from_equation(Tree*, std::string);
	void build_equation_from_tree(std::string, Tree*);
};

#endif // PARSER_HPP
