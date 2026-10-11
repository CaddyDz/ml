#ifndef DEFS_HPP
#define DEFS_HPP

#include <map>
#include <string>
#include <regex>

// Vocabulary shared by all the modules: the kinds of node, the operators and the
// functions, with the tables that translate them to and from text.

/// Binary operators. In equations ^ binds tightest and is right-associative.
enum class OPERATOR
{
	ADD=0,
	MINUS,
	PROD,
	DIV,
	POW
};

/// Functions, written _name[argument] in equations (names in fn2str).
/// LN is the natural logarithm and LOG the base-10 one.
/// MIN and MAX take two arguments (_min[a,b]), the others one.
enum class FUNCTION
{
	EXP=0,
	MIN,
	MAX,
	COS,
	SIN,
	LOG,
	LN,
	ROOTSQUARE

};

static std::map<OPERATOR, std::string> op2str = {
	{OPERATOR::ADD,"+"},
	{OPERATOR::MINUS,"-"},
	{OPERATOR::PROD,"*"},
	{OPERATOR::DIV,"/"},
	{OPERATOR::POW,"^"},
};

static std::map<std::string, OPERATOR> str2op = {
	{"+", OPERATOR::ADD},
	{"-", OPERATOR::MINUS},
	{"*",OPERATOR::PROD},
	{"/", OPERATOR::DIV},
	{"^", OPERATOR::POW},
};

static std::map<FUNCTION, std::string> fn2str = {
	{FUNCTION::EXP,"exp"},
	{FUNCTION::SIN,"sin"},
	{FUNCTION::COS,"cos"},
	{FUNCTION::MIN,"min"},
	{FUNCTION::MAX,"max"},
	{FUNCTION::LN,"ln"},
	{FUNCTION::LOG,"log"},
	{FUNCTION::ROOTSQUARE,"sqrt"}
};

static std::map<std::string,FUNCTION> str2fn = {
	{"exp",FUNCTION::EXP},
	{"sin",FUNCTION::SIN},
	{"cos",FUNCTION::COS},
	{"min",FUNCTION::MIN},
	{"max",FUNCTION::MAX},
	{"sqrt",FUNCTION::ROOTSQUARE},
	{"ln",FUNCTION::LN},
	{"log",FUNCTION::LOG}
};

/// Kind of a node, which also tells the real class of a Node*:
/// CONSTANT is a tNode<double>, VARIABLE a tNode<std::string>,
/// OPERATOR a tNode<OPERATOR> and FUNCTION a tNode<FUNCTION>.
enum class NodeType{
	NONE=-1,
	CONSTANT = 0,
	VARIABLE,
	OPERATOR,
	FUNCTION
};

// Token predicates, used by the parser and the tree search functions
bool is_number(const std::string&);    ///< 3, 2.5, .5, 1e3 or 1e-3 (no sign); rejected if it overflows or underflows a double
bool is_operator( const std::string&); ///< one of + - * / ^
bool is_func( const std::string&);     ///< a function name such as "sin" (no underscore)
bool is_variable(const std::string&);  ///< letters only, and not a function name

/// Value of a token accepted by is_number. Unlike std::stod it does not depend on
/// the C locale, which QApplication sets from the system.
double to_number(const std::string&);

#endif // DEFS_HPP

