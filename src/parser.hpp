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
#include <sstream>
#include <map>
#include "tree.hpp"

/**
 * Infix equation parser: turns a string such as "3+4*_exp[3+x]/(1-5)^2" into a Tree.
 *
 * Syntax: numbers (2.5, 1e3), variables (letters only), + - * / ^, parentheses,
 * a sign at the start of an expression or group (-x, 2*(-x)) and functions written
 * _name[argument]: _exp _ln _log _sqrt _sin _cos, plus _min[a,b] and _max[a,b].
 * ^ binds tightest and is right-associative, the other operators are
 * left-associative. Spaces may separate tokens but not split one. See the README.
 *
 * Function calls and groups between parenthesis are first replaced by
 * placeholders (#funcN#, #exprN#) kept in m_list_func and m_list_expr, then the
 * operators are split by increasing precedence, recursively.
 */
class CParser
{
private:
	std::map<std::string, std::string> m_list_expr;
	std::map<std::string, std::pair<FUNCTION, std::string> > m_list_func;

	Node* parse_expression(std::string);
	bool parse_operator_level(std::string&, const std::regex&, bool, Node*&);
	Node* parse_function_call(const std::pair<FUNCTION, std::string>&);

public:
	CParser(){}
	CParser(const CParser&) = delete;
	~CParser(){}

	void print_expr();   ///< debug: placeholders exprN of the last equation
	void print_func();   ///< debug: placeholders funcN of the last equation

	/// Builds the subtree of an equation; nullptr (and a message on stderr) if it is invalid.
	Node* parse(std::string);

	// Replace the groups (...) / the calls _name[...] by placeholders; false if unbalanced
	bool parse_factor_between_parenthesis(std::string&);
	bool parse_function(std::string&);

	/// Clears the tree and fills it with the equation. If the equation is invalid,
	/// a message is printed on stderr and the tree is left empty.
	void build_tree_from_equation(Tree*, std::string);
};

#endif // PARSER_HPP
