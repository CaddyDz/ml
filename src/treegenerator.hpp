#ifndef TREEGENERATOR_HPP
#define TREEGENERATOR_HPP

#include <random>
#include <cstdlib>
#include "tree.hpp"

typedef struct
{
	unsigned int max_depth;
	unsigned int max_polynomial_order;
	std::vector<std::string> m_list_variables;
	double min_number;
	double max_number;

} Rules;

class CTreeGenerator
{
private:
	Rules m_rules;
	void generate_node(Node*&, unsigned int) const;

public:
	CTreeGenerator() = delete;
	CTreeGenerator(const Rules&);
	CTreeGenerator(const CTreeGenerator&) = delete;
	~CTreeGenerator(){}

	Rules rules() const{return m_rules;}
	Tree* generate_tree() const;
	bool check_rules(Tree*) const;
};

#endif // TREEGENERATOR_H
