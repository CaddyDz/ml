#ifndef DEFS_HPP
#define DEFS_HPP

#include <map>
#include <string>
#include <regex>

enum class OPERATOR
{
	ADD=0,
	MINUS,
    PROD,
	DIV,
	POW
};

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

enum class NodeType{
    NONE=-1,
    CONSTANT = 0,
    VARIABLE,
    OPERATOR,
    FUNCTION
};

bool is_number(const std::string&);
bool is_operator( const std::string&);
bool is_func( const std::string&);
bool is_variable(const std::string&);

#endif // DEFS_HPP

