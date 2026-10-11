// Regression tests for the non-Qt core (node, tree, parser, generator, classifier).
// Build and run with `make check` from this directory; see the README.

#include <algorithm>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cmath>
#include <functional>
#include <limits>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <set>
#include <type_traits>
#include <vector>

#include "classifier.hpp"
#include "parser.hpp"
#include "tree.hpp"
#include "treegenerator.hpp"

// owning types must not be copyable: a copy would delete the same nodes twice
static_assert(!std::is_copy_constructible<Node>::value && !std::is_copy_assignable<Node>::value, "Node is copyable");
static_assert(!std::is_copy_constructible<Tree>::value && !std::is_copy_assignable<Tree>::value, "Tree is copyable");
static_assert(!std::is_copy_constructible<CClassifier>::value && !std::is_copy_assignable<CClassifier>::value,
			  "CClassifier is copyable");

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond) \
	do { \
		++g_checks; \
		if (!(cond)) { \
			++g_failures; \
			std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond << std::endl; \
		} \
	} while (0)

#define CHECK_NEAR(actual, expected) \
	do { \
		++g_checks; \
		double a_ = (actual), e_ = (expected); \
		if (!(std::fabs(a_ - e_) <= 1e-9 * std::max(1.0, std::fabs(e_)))) { \
			++g_failures; \
			std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK_NEAR failed: " #actual " = " << a_ \
					  << ", expected " << e_ << std::endl; \
		} \
	} while (0)

typedef std::map<std::string, double> Vars;

// parses 'equation' into a fresh tree and evaluates it
static double value_of(const std::string& equation, const Vars& vars = Vars())
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, equation);
	return tree.compute_value(vars);
}

static bool parses(const std::string& equation)
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, equation);
	return !tree.isEmpty();
}

// the classifier reports every tree it draws on std::cout: keep the test output readable
struct QuietCout
{
	std::ostringstream sink;
	std::streambuf* previous = std::cout.rdbuf(sink.rdbuf());
	~QuietCout() { std::cout.rdbuf(previous); }
};

// preorder text of a subtree without the constant values, to compare tree shapes
static std::string shape(Node* node)
{
	if (node == nullptr)
		return ".";

	std::ostringstream os;
	os << static_cast<int>(node->type());

	switch (node->type())
	{
	case NodeType::VARIABLE: os << static_cast<tNode<std::string>*>(node)->data(); break;
	case NodeType::OPERATOR: os << static_cast<int>(static_cast<tNode<OPERATOR>*>(node)->data()); break;
	case NodeType::FUNCTION: os << static_cast<int>(static_cast<tNode<FUNCTION>*>(node)->data()); break;
	default: break;
	}

	return os.str() + "(" + shape(node->left()) + "," + shape(node->right()) + ")";
}

static Node* constant(double value)
{
	Node* node = new tNode<double>(value);
	node->s_type(NodeType::CONSTANT);
	return node;
}

static Node* variable(const std::string& name)
{
	Node* node = new tNode<std::string>(name);
	node->s_type(NodeType::VARIABLE);
	return node;
}

static void test_precedence_and_associativity()
{
	CHECK_NEAR(value_of("2+3*4"), 14);
	CHECK_NEAR(value_of("(2+3)*4"), 20);
	CHECK_NEAR(value_of("10-4-3"), 3);
	CHECK_NEAR(value_of("100/10/5"), 2);
	CHECK_NEAR(value_of("2*3/4"), 1.5);
	CHECK_NEAR(value_of("8/4*2"), 4);
	CHECK_NEAR(value_of("2^3*2"), 16);
	CHECK_NEAR(value_of("2^2^3"), 256);              // right-associative: 2^(2^3)
	CHECK_NEAR(value_of("(1-5)^2^3"), 65536);
	CHECK_NEAR(value_of("3+4*2/(1-5)^2^3-6*z", {{"z", 2}}), 3 + 8.0/65536 - 12);
	CHECK_NEAR(value_of(" 1 + 2 * x ", {{"x", 3}}), 7);
	CHECK(value_of("0.1*3/3") == (0.1*3)/3);                           // left to right, not 0.1*(3/3)
	CHECK_NEAR(value_of("2e3 - (1e-3) * x", {{"x", 1000}}), 1999);       // spaces around operators are fine

	// the public parse() normalizes its input too
	CParser parser;
	Node* node = parser.parse(" x + 1 ");
	CHECK(node != nullptr);
	delete node;
}

static void test_decimals_ignore_the_c_locale()
{
	// QApplication sets the C locale from the system: a French one uses a decimal comma
	const char* previous = std::setlocale(LC_ALL, nullptr);
	std::string saved = previous ? previous : "C";
	int tested = 0;

	for (const char* name : {"fr_FR.UTF-8", "fr_FR", "de_DE.UTF-8", "de_DE", "C.UTF-8"})
	{
		if (std::setlocale(LC_ALL, name) == nullptr)
			continue;

		tested += (std::localeconv()->decimal_point[0] == ',');

		CHECK_NEAR(value_of("2.5*x", {{"x", 2}}), 5);
		CHECK_NEAR(value_of(".5+(1e-1)*1.5e1"), 2);
	}

	// only ASCII spaces are removed: under a UTF-8 C locale a non-breaking space
	// byte must not glue two terms together
	if (std::setlocale(LC_ALL, "en_US.UTF-8") != nullptr || std::setlocale(LC_ALL, "C.UTF-8") != nullptr)
	{
		CHECK(!parses("x\xA0y"));
		CHECK(!parses("1\xA0" "5"));
	}

	std::setlocale(LC_ALL, saved.c_str());

	if (tested == 0)
		std::cout << "       (no locale with a decimal comma is installed: only the C locale was checked)" << std::endl;
}

static void test_unary_minus()
{
	CHECK_NEAR(value_of("-x", {{"x", 3}}), -3);
	CHECK_NEAR(value_of("-x^2", {{"x", 3}}), -9);
	CHECK_NEAR(value_of("(-3)^2"), 9);
	CHECK_NEAR(value_of("2*(-x)", {{"x", 4}}), -8);
	CHECK_NEAR(value_of("+5-2"), 3);
}

static void test_functions()
{
	CHECK_NEAR(value_of("_ln[8]"), std::log(8.0));
	CHECK_NEAR(value_of("_log[1000]"), 3);
	CHECK_NEAR(value_of("_sqrt[16]"), 4);
	CHECK_NEAR(value_of("_exp[0]"), 1);
	CHECK_NEAR(value_of("_exp[3+x]", {{"x", -3}}), 1);
	CHECK_NEAR(value_of("_sin[x]*_cos[x]", {{"x", 0.5}}), std::sin(0.5) * std::cos(0.5));
	CHECK_NEAR(value_of("_sqrt[_exp[0]+3]"), 2);
	CHECK_NEAR(value_of("(1+_cos[2*x])^2", {{"x", 0}}), 4);
	CHECK_NEAR(value_of("_min[x,3]", {{"x", 5}}), 3);
	CHECK_NEAR(value_of("_max[(x+1),2*y]", {{"x", 1}, {"y", 4}}), 8);
	CHECK_NEAR(value_of("_max[_min[x,1],0]", {{"x", 0.5}}), 0.5);
}

static void test_malformed_equations_are_rejected()
{
	// each of these used to crash, hang or silently build a wrong tree
	const char* bad[] = {"", "(x+1", "x+1)", "_sin[x", "_sin x", "x+", "x+y+", "2x", "_tan[x]", "_min[x]", "()", "*x",
						 "1e999", "x*1e999", "x+y+-z", "2*x**2", "a/b//c",
						 // the placeholders of groups and calls cannot be named or forged by the input
						 "(expr0)", "_sin[func0]", "(x)+expr0", "(a)+((b)2+(c)+(d)+(e)+(f)+(g)+(h)+(i)+(j)+(k)+(l))", "(#expr0#)",
						 // spaces cannot glue two tokens together
						 "x y", "1 2", "1 e5", "_s in[x]", "1e - 3", "1e -3", "1e- 3", "(2e +3)",
						 // empty groups and calls
						 "x*()", "_exp[]", "_sin[]*x"};

	for (const char* equation : bad)
		CHECK(!parses(equation));
}

static void test_tree_reuse_resets_state()
{
	CParser parser;
	Tree tree;

	parser.build_tree_from_equation(&tree, "((x+y)*(x-y))^2");
	CHECK(tree.depth() == 4);

	parser.build_tree_from_equation(&tree, "2*z");
	CHECK(tree.depth() == 2);
	CHECK_NEAR(tree.compute_value({{"z", 3}}), 6);   // x and y are not required anymore

	// an invalid equation leaves the tree empty, with nothing left of the previous one
	parser.build_tree_from_equation(&tree, "((x+y)*(x-y))^2");
	parser.build_tree_from_equation(&tree, "x+");
	CHECK(tree.isEmpty() && tree.depth() == 0 && tree.nodes_from_pos().empty());
	CHECK(std::isnan(tree.compute_value(Vars())));

	parser.build_tree_from_equation(&tree, "((x+y)*(x-y))^2");
	tree.clear(true);
	CHECK(tree.isEmpty() && tree.depth() == 0);
}

static void test_missing_variable_is_nan()
{
	CHECK(std::isnan(value_of("x+y", {{"x", 1}})));
}

static void test_invalid_values_propagate()
{
	// NaN means "no valid value": it must survive every operation
	CHECK_NEAR(value_of("x/0", {{"x", 3}}), 0);                      // protected division
	CHECK(std::isnan(value_of("_sqrt[x]/0", {{"x", -1}})));
	CHECK(std::isnan(value_of("_max[_sqrt[x],1]", {{"x", -1}})));
	CHECK(std::isnan(value_of("_max[1,_sqrt[x]]", {{"x", -1}})));
	CHECK(std::isnan(value_of("_min[_sqrt[x],1]", {{"x", -1}})));
	CHECK(std::isnan(value_of("_min[1,_sqrt[x]]", {{"x", -1}})));
	CHECK(std::isinf(value_of("_exp[x]/0", {{"x", 1000}})));          // an infinity stays infinite
	CHECK(std::isnan(value_of("_sqrt[x]^0", {{"x", -1}})));             // pow(NaN, 0) would be 1
	CHECK(std::isnan(value_of("1^_ln[x]", {{"x", -1}})));               // pow(1, NaN) would be 1

	// a missing operand has no value
	Tree broken;
	Node* minus = new tNode<OPERATOR>(OPERATOR::MINUS);
	minus->s_type(NodeType::OPERATOR);
	minus->s_left(constant(1.0));
	broken.makeRoot(minus);
	broken.build();
	CHECK(std::isnan(broken.compute_value(Vars())));

	// a variable renamed without build() is not silently 0
	CParser parser;
	Tree renamed;
	parser.build_tree_from_equation(&renamed, "x*2+1");
	static_cast<tNode<std::string>*>(renamed.find_node_from_val("x"))->s_data("y");
	CHECK(std::isnan(renamed.compute_value({{"x", 1}, {"y", 5}})));
	renamed.build();
	CHECK_NEAR(renamed.compute_value({{"y", 5}}), 11);

	Tree empty;
	CHECK(std::isnan(empty.compute_value(Vars())));
	CHECK(std::isnan(value_of("x+")));                               // nothing parsed
}

static void test_data()
{
	double result = 2;
	CData data({{"x", 1.0}}, result);

	CHECK_NEAR(data.get_data("x"), 1);
	CHECK(std::isnan(data.get_data("y")));

	data.s_result(5);
	CHECK_NEAR(data.get_result(), 5);

	data.s_data({{"y", 3.0}});
	CHECK_NEAR(data.get_data("y"), 3);
}

static void test_queries()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "3+4*_exp[3+x]/(1-5)^2^3-6*z");

	CHECK(tree.nodes_at_depth(99).empty());
	CHECK(tree.nodes_at_depth(1).size() == 1);

	Node* four = tree.find_node_from_val("4");
	CHECK(four != nullptr && four->type() == NodeType::CONSTANT);

	Node* div = tree.find_node_from_val("/");
	CHECK(div != nullptr && div->type() == NodeType::OPERATOR);
	CHECK(div != nullptr && tree.find_node_from_val_at_depth("/", div->depth()) == div);

	Node* exp = tree.find_node_from_val("exp");
	CHECK(exp != nullptr && exp->type() == NodeType::FUNCTION);

	Node* z = tree.find_node_from_val("z");
	CHECK(z != nullptr && z->type() == NodeType::VARIABLE);

	CHECK(tree.find_node_from_val("7") == nullptr);
	CHECK(tree.find_node_from_val_at_depth("4", 1) == nullptr);
	CHECK(four != nullptr && tree.find_node_from_val_at_depth("4", four->depth()) == four);
	CHECK(tree.find_node_from_val("1e999") == nullptr);
	CHECK(tree.find_node_from_val_at_depth("1e999", 1) == nullptr);
	CHECK(tree.find_node_from_val("w") == nullptr);
}

static void test_search_by_node_type()
{
	// negative constants and variable names that are not plain letters exist in
	// generated trees: the node's own type decides how the value is read
	Node* minus_three = constant(-3.0);
	Node* x1 = variable("x1");
	Node* ln = variable("ln");
	Node* sum = new tNode<OPERATOR>(OPERATOR::ADD, x1, ln);

	Tree tree;
	tree.makeRoot(new tNode<OPERATOR>(OPERATOR::PROD, minus_three, sum));
	tree.build();

	CHECK(tree.find_node_from_val("-3") == minus_three);
	CHECK(tree.find_node_from_val("-3.0") == minus_three);
	CHECK(tree.find_node_from_val("3") == nullptr);
	CHECK(tree.find_node_from_val("x1") == x1);
	CHECK(tree.find_node_from_val_at_depth("x1", 3) == x1);
	CHECK(tree.find_node_from_val("ln") == ln);
	CHECK(tree.find_node_from_val("*") == tree.root());
	CHECK(tree.find_node_from_val("") == nullptr);
	CHECK(tree.find_node_from_val("-") == nullptr);
}

static void test_operator_constructor_links_children()
{
	// same construction as the first tree of main.cpp
	Node* sum_l = constant(4.0);
	Node* sum_r = variable("y");
	Node* op = new tNode<OPERATOR>(OPERATOR::ADD, sum_l, sum_r);

	CHECK(sum_l->parent() == op && sum_r->parent() == op);
	CHECK(sum_l->depth() == 2 && sum_r->depth() == 2);

	Tree tree;
	tree.makeRoot(op);
	tree.build();

	tree.replace(sum_l, constant(1.0));                // needs the parent pointer
	CHECK_NEAR(tree.compute_value({{"y", 2}}), 3);
}

static void test_replace()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "(3*(x+1))^2");

	// crossover: graft a copy of another tree's root, both trees stay independent
	Tree donor;
	parser.build_tree_from_equation(&donor, "_sqrt[x]");

	Node* gene = tree.find_node_from_val_at_depth("+", 3);
	CHECK(gene != nullptr);
	tree.replace(gene, donor.root()->clone());
	CHECK_NEAR(tree.compute_value({{"x", 4}}), 36);    // (3*sqrt(4))^2
	CHECK_NEAR(donor.compute_value({{"x", 9}}), 3);

	// replacing the root
	tree.replace(tree.root(), constant(5.0));
	CHECK(tree.depth() == 1);
	CHECK_NEAR(tree.compute_value(Vars()), 5);

	// a node cannot replace its own ancestor: it would be deleted with it
	parser.build_tree_from_equation(&tree, "(x+1)*2");
	Node* plus = tree.find_node_from_val("+");
	CHECK(!tree.replace(tree.root(), plus));
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 4);     // unchanged

	// nor can a new subtree that contains nodes of the tree
	Node* cos = new tNode<FUNCTION>(FUNCTION::COS);
	cos->s_type(NodeType::FUNCTION);
	cos->s_left(plus);                                  // plus is still the tree's
	CHECK(!tree.replace(plus, cos));
	cos->s_left(nullptr);
	plus->s_parent(tree.root());
	delete cos;

	Node* two = constant(2.0);
	Node* minus = new tNode<OPERATOR>(OPERATOR::MINUS, two, tree.root());
	CHECK(!tree.replace(tree.root(), minus));
	minus->s_children(nullptr, nullptr);
	two->s_parent(nullptr);
	tree.root()->s_parent(nullptr);
	delete minus;
	delete two;
	tree.build();
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 4);     // still unchanged

	// a refused node stays the caller's
	Node* lost = constant(7.0);
	CHECK(!tree.replace(nullptr, lost));
	delete lost;
}

static void test_remove_intermediate()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "(1+_cos[2*x])^2");

	Node* cos = tree.find_node_from_val("cos");
	tree.remove_intermediate(cos, 1);                    // cos has no right child: no-op
	CHECK_NEAR(tree.compute_value({{"x", 0}}), 4);

	tree.remove_intermediate(cos);                       // (1+2*x)^2
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 9);
	CHECK(tree.find_node_from_val("cos") == nullptr);

	tree.remove_intermediate(tree.root(), 0);            // the root is replaced by 1+2*x
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 3);
	CHECK(tree.depth() == 3);

	// a left operand: cos(x)+1 becomes x+1
	parser.build_tree_from_equation(&tree, "_cos[x]+1");
	tree.remove_intermediate(tree.find_node_from_val("cos"));
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 2);
}

static void test_expand_from_root()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "x+y");

	Node* cos = new tNode<FUNCTION>(FUNCTION::COS);
	cos->s_type(NodeType::FUNCTION);
	CHECK(tree.expand_from_root(cos));

	CHECK(tree.root() == cos);
	CHECK(tree.depth() == 3);
	CHECK_NEAR(tree.compute_value({{"x", 0}, {"y", 0}}), 1);

	// an operator keeps its right operand: cos(x+y)*2
	Node* times = new tNode<OPERATOR>(OPERATOR::PROD);
	times->s_type(NodeType::OPERATOR);
	times->s_right(constant(2.0));
	CHECK(tree.expand_from_root(times));
	CHECK_NEAR(tree.compute_value({{"x", 0}, {"y", 0}}), 2);

	// a node of the tree is refused (it would create a cycle)
	CHECK(!tree.expand_from_root(tree.find_node_from_val("x")));
	CHECK(tree.root() == times);

	// refused: a right operand taken from the tree, an operator without its right
	// operand, a node that already has a left child, a leaf, and an empty tree to wrap
	Node* y = tree.find_node_from_val("y");
	Node* y_parent = y->parent();
	Node* plus = new tNode<OPERATOR>(OPERATOR::ADD);
	plus->s_type(NodeType::OPERATOR);
	plus->s_right(y);
	CHECK(!tree.expand_from_root(plus));
	plus->s_right(nullptr);
	y->s_parent(y_parent);
	tree.build();
	CHECK(!tree.expand_from_root(plus));
	delete plus;

	Node* cos_one = new tNode<FUNCTION>(FUNCTION::COS);
	cos_one->s_type(NodeType::FUNCTION);
	cos_one->s_left(constant(1.0));
	CHECK(!tree.expand_from_root(cos_one));
	delete cos_one;

	Node* leaf = constant(3.0);
	CHECK(!tree.expand_from_root(leaf));
	delete leaf;

	Tree empty;
	Node* sin = new tNode<FUNCTION>(FUNCTION::SIN);
	sin->s_type(NodeType::FUNCTION);
	CHECK(!empty.expand_from_root(sin));
	delete sin;

	CHECK(tree.root() == times);
	CHECK_NEAR(tree.compute_value({{"x", 0}, {"y", 0}}), 2);
}

static void test_positions()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "(x+1)*2");

	const std::map<int, Node*>& pos = tree.nodes_from_pos();
	CHECK(pos.size() == 7);
	CHECK(pos.at(0) == tree.root());
	CHECK(pos.at(1) == tree.find_node_from_val("+"));
	CHECK(pos.at(2) == tree.find_node_from_val("2"));
	CHECK(pos.at(3) == tree.find_node_from_val("x"));
	CHECK(pos.at(4) == tree.find_node_from_val("1"));
	CHECK(pos.at(5) == nullptr && pos.at(6) == nullptr);   // children of the leaf 2

	// only the levels whose positions fit in an int are indexed (depths up to 31)
	std::string equation = "x";
	for (int i = 0; i < 40; ++i)
		equation += "+1";
	parser.build_tree_from_equation(&tree, equation);

	const std::map<int, Node*>& deep = tree.nodes_from_pos();
	std::size_t shallow_nodes = 0;
	for (Node* node : tree.nodes())
		shallow_nodes += (node->depth() <= 31);

	std::size_t indexed = 0;
	for (const auto& entry : deep)
		indexed += (entry.second != nullptr);

	CHECK(indexed == shallow_nodes);
	CHECK(deep.count((1 << 30) - 1) == 1);                  // leftmost node at depth 31
	CHECK(deep.rbegin()->first <= std::numeric_limits<int>::max());
}

static void test_clone_is_deep()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "_max[x,2]*(y-1)");

	Tree copy;
	copy.makeRoot(tree.root()->clone());
	copy.build();

	CHECK(copy.root() != tree.root());
	CHECK(copy.nodes().size() == tree.nodes().size());
	CHECK_NEAR(copy.compute_value({{"x", 3}, {"y", 2}}), 3);

	parser.build_tree_from_equation(&tree, "0");        // frees the original nodes
	CHECK_NEAR(copy.compute_value({{"x", 3}, {"y", 2}}), 3);

	// the copy is linked both ways, so it can be mutated after a crossover
	for (Node* node : copy.nodes())
	{
		CHECK(node == copy.root() || node->parent() != nullptr);
		CHECK(node->left() == nullptr || node->left()->parent() == node);
		CHECK(node->right() == nullptr || node->right()->parent() == node);
	}

	Tree host, donor;
	parser.build_tree_from_equation(&host, "x+1");
	parser.build_tree_from_equation(&donor, "_sqrt[y*2]");
	CHECK(host.replace(host.find_node_from_val("1"), donor.root()->clone()));
	CHECK(host.replace(host.find_node_from_val("*"), constant(4.0)));   // inside the graft
	CHECK_NEAR(host.compute_value({{"x", 1}}), 3);
	CHECK_NEAR(donor.compute_value({{"y", 8}}), 4);                      // the donor is intact
}

static void test_build_is_linear()
{
	// a long sum is a left-deep tree: building it used to cost 2^depth
	std::string equation = "x";
	for (int i = 0; i < 300; ++i)
		equation += "+1";

	auto start = std::chrono::steady_clock::now();
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, equation);
	double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

	CHECK(tree.depth() == 301);
	CHECK_NEAR(tree.compute_value({{"x", 0}}), 300);
	CHECK(seconds < 5.0);

	// a long chain of ^ is split once, not re-scanned for every operator
	std::string tower = "x";
	for (int i = 0; i < 500; ++i)
		tower += "^x";

	start = std::chrono::steady_clock::now();
	parser.build_tree_from_equation(&tree, tower);
	seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

	CHECK(tree.depth() == 501);
	CHECK_NEAR(tree.compute_value({{"x", 1}}), 1);
	CHECK(seconds < 5.0);
	CHECK_NEAR(value_of("2^3^2"), 512);                                 // still a^(b^c)
}

static void test_generator_rules()
{
	// max_depth below 2, no variable and polynomial order 1 used to recurse
	// without bound or divide by zero
	Rules rules{1, 1, {}, -5, 5};
	CTreeGenerator generator(rules);
	CHECK(generator.rules().max_depth == 2);
	CHECK(generator.rules().max_polynomial_order == 1);

	CTreeGenerator generator_order0(Rules{3, 0, {"x"}, -5, 5});
	CHECK(generator_order0.rules().max_polynomial_order == 1);

	for (int i = 0; i < 300; ++i)
		delete generator_order0.generate_tree();

	for (int i = 0; i < 200; ++i)
	{
		Tree* tree = generator.generate_tree();
		CHECK(tree->depth() <= 2);
		CHECK(tree->nodes().size() == 3);
		delete tree;
	}

	Rules rules2{4, 3, {"x", "y"}, -50, 50};
	CTreeGenerator generator2(rules2);

	for (int i = 0; i < 500; ++i)
	{
		Tree* tree = generator2.generate_tree();
		CHECK(tree->depth() >= 2 && tree->depth() <= 4);

		// every exponent is a whole number between 1 and max_polynomial_order
		for (Node* node : tree->nodes())
		{
			if (node->type() != NodeType::OPERATOR || static_cast<tNode<OPERATOR>*>(node)->data() != OPERATOR::POW)
				continue;

			CHECK(node->right() != nullptr && node->right()->type() == NodeType::CONSTANT);

			if (node->right() != nullptr && node->right()->type() == NodeType::CONSTANT)
			{
				double exponent = static_cast<tNode<double>*>(node->right())->data();
				CHECK(exponent == std::round(exponent) && exponent >= 1 && exponent <= 3);
			}
		}

		delete tree;
	}

	// order 1 used to divide by zero, and the largest order was never drawn
	Rules linear{3, 1, {"x"}, -5, 5};
	Rules cubic{4, 3, {"x", "y"}, -50, 50};
	CTreeGenerator generator_linear(linear);
	CTreeGenerator generator_cubic(cubic);
	bool largest_order_drawn = false;

	for (int i = 0; i < 500; ++i)
	{
		for (int order = 1; order <= 3; order += 2)
		{
			Tree* tree = (order == 1 ? generator_linear : generator_cubic).generate_tree();

			for (Node* node : tree->nodes())
			{
				if (node->type() == NodeType::OPERATOR && static_cast<tNode<OPERATOR>*>(node)->data() == OPERATOR::POW)
				{
					double exponent = static_cast<tNode<double>*>(node->right())->data();
					CHECK(exponent >= 1 && exponent <= order);
					largest_order_drawn |= (order == 3 && exponent == 3);
				}
			}

			delete tree;
		}
	}

	CHECK(largest_order_drawn);

	// an exponent that is not a constant is replaced by a random order in [1, max]:
	// the largest order must be drawn too
	CTreeGenerator generator_orders(Rules{3, 3, {"x"}, -5, 5});
	bool drawn[4] = {false, false, false, false};

	for (int i = 0; i < 300; ++i)
	{
		CParser parser;
		Tree tree;
		parser.build_tree_from_equation(&tree, "x^y");
		generator_orders.check_rules(&tree);

		Node* exponent = tree.root()->right();
		CHECK(exponent->type() == NodeType::CONSTANT);
		double order = static_cast<tNode<double>*>(exponent)->data();
		CHECK(order >= 1 && order <= 3);

		if (order >= 1 && order <= 3)
			drawn[static_cast<int>(order)] = true;
	}

	CHECK(drawn[1] && drawn[2] && drawn[3]);

	// constant exponents become whole orders in [1, max] (NaN and 0 included)
	CTreeGenerator generator_three(Rules{3, 3, {"x"}, -5, 5});

	for (double exponent : {2.5, -2.0, 0.0, 7.0, 1.0, std::nan("")})
	{
		Tree power;
		Node* pow = new tNode<OPERATOR>(OPERATOR::POW, variable("x"), constant(exponent));
		power.makeRoot(pow);
		power.build();
		generator_three.check_rules(&power);

		double order = static_cast<tNode<double>*>(power.root()->right())->data();
		CHECK(order == std::round(order) && order >= 1 && order <= 3);

		if (exponent == 1.0 || exponent == 2.5)
			CHECK(order == std::round(exponent));                         // kept, or rounded
	}

	// a division by the constant 0 is fixed even when the range of constants is {0}
	CTreeGenerator generator_zero(Rules{3, 2, {"x"}, 0, 0});
	CParser parser;
	Tree division;
	parser.build_tree_from_equation(&division, "x/0");
	generator_zero.check_rules(&division);
	CHECK(static_cast<tNode<double>*>(division.root()->right())->data() != 0.0);

	// the constants come from a random engine: they must not all be the same value
	std::vector<double> constants;
	CTreeGenerator generator_constants(Rules{4, 2, {"x"}, -50, 50});

	for (int i = 0; i < 50; ++i)
	{
		Tree* tree = generator_constants.generate_tree();

		for (Node* node : tree->nodes())
		{
			bool exponent = node->parent() != nullptr && node->parent()->right() == node &&
							node->parent()->type() == NodeType::OPERATOR &&
							static_cast<tNode<OPERATOR>*>(node->parent())->data() == OPERATOR::POW;

			if (node->type() == NodeType::CONSTANT && !exponent)
				constants.push_back(static_cast<tNode<double>*>(node)->data());
		}

		delete tree;
	}

	std::sort(constants.begin(), constants.end());
	CHECK(constants.size() >= 2 && std::unique(constants.begin(), constants.end()) - constants.begin() >= 2);

	// a range of constants that is not finite falls back to [-1, 1]
	for (Rules invalid : {Rules{3, 3, {"x"}, std::nan(""), 5}, Rules{3, 3, {"x"}, -1e308, 1e308}})
	{
		CTreeGenerator generator4(invalid);

		for (int i = 0; i < 100; ++i)
		{
			Tree* tree = generator4.generate_tree();

			for (Node* node : tree->nodes())
			{
				if (node->type() != NodeType::CONSTANT)
					continue;

				double value = static_cast<tNode<double>*>(node)->data();
				bool exponent = node->parent() != nullptr && node->parent()->type() == NodeType::OPERATOR &&
								static_cast<tNode<OPERATOR>*>(node->parent())->data() == OPERATOR::POW &&
								node->parent()->right() == node;
				CHECK(std::isfinite(value));
				CHECK(exponent ? (value >= 1 && value <= 3) : (value >= -1 && value <= 1));
			}

			delete tree;
		}
	}

	// a generator created right after another one must not replay its trees
	Rules rules3{5, 2, {"x", "y"}, -10, 10};
	std::string shapes[2];

	for (std::string& generated : shapes)
	{
		CTreeGenerator generator3(rules3);

		for (int i = 0; i < 10; ++i)
		{
			Tree* tree = generator3.generate_tree();
			generated += shape(tree->root());
			delete tree;
		}
	}

	CHECK(shapes[0] != shapes[1]);
}

static void test_classifier()
{
	// the five samples of main.cpp, target x^2 - y + 3
	CData d1({{"x", 0.0}, {"y", -3.0}}, 6);
	CData d2({{"x", 1.0}, {"y", -1.0}}, 5);
	CData d3({{"x", 2.0}, {"y", 3.0}}, 4);
	CData d4({{"x", 3.0}, {"y", 9.0}}, 3);
	CData d5({{"x", 4.0}, {"y", 17.0}}, 2);

	Tree target;
	CParser parser;
	parser.build_tree_from_equation(&target, "x^2-y+3");

	for (CData* data : {&d1, &d2, &d3, &d4, &d5})
		CHECK_NEAR(target.compute_value(data->get_data()), data->get_result());

	QuietCout quiet;

	CClassifier classifier;
	classifier.s_nb_elm_per_gen(10);
	for (CData* data : {&d1, &d2, &d3, &d4, &d5})
		classifier.s_data(data);

	Rules rules{4, 2, {"x", "y"}, -50, 50};
	CTreeGenerator generator(rules);
	classifier.compute_new_generation(&generator);

	CHECK(classifier.get_tree_for_specific_generation(0, 0) != nullptr);
	CHECK(classifier.get_tree_for_specific_generation(9, 0) != nullptr);
	CHECK(classifier.get_tree_for_specific_generation(10, 0) == nullptr);
	CHECK(classifier.get_tree_for_specific_generation(0, 1) == nullptr);
	CHECK(classifier.get_tree_for_specific_generation(3, 0) != classifier.get_tree_for_specific_generation(4, 0));

	classifier.print_generation(1);      // out of range: prints nothing
	classifier.print_generation(-1);

	// huge but finite errors give finite scores: the sum of their squares must not overflow
	CData huge1({{"x", 1.0}}, 2e154);
	CData huge2({{"x", 2.0}}, 4e154);
	CClassifier wide;
	wide.s_nb_elm_per_gen(5);
	wide.s_data(&huge1);
	wide.s_data(&huge2);
	CTreeGenerator generator_small(Rules{3, 2, {"x"}, -10, 10});
	wide.compute_new_generation(&generator_small);
	CHECK(wide.get_tree_for_specific_generation(4, 0) != nullptr);

	// a score too large for a double is rejected
	CData far1({{"x", 1.0}}, 1.5e308);
	CData far2({{"x", 2.0}}, -1.5e308);
	CClassifier too_far;
	too_far.s_nb_elm_per_gen(2);
	too_far.s_data(&far1);
	too_far.s_data(&far2);
	too_far.compute_new_generation(&generator_small);
	CHECK(too_far.get_tree_for_specific_generation(0, 0) == nullptr);

	// the score logged for each kept tree is the one that is stored and printed
	{
		QuietCout capture;
		CClassifier logged;
		logged.s_verbose(true);
		logged.s_nb_elm_per_gen(5);
		for (CData* data : {&d1, &d2, &d3, &d4, &d5})
			logged.s_data(data);
		logged.compute_new_generation(&generator);

		// the last value logged for each tree number is the kept one
		std::map<int, std::string> kept;
		std::istringstream lines(capture.sink.str());
		std::string line;
		while (std::getline(lines, line))
		{
			int number;
			char value[64];
			if (std::sscanf(line.c_str(), "NB TREES: %d ERROR: %63s", &number, value) == 2)
				kept[number] = value;
		}

		capture.sink.str("");
		logged.print_generation(0);

		std::vector<std::string> logged_scores, printed_scores;
		for (const auto& entry : kept)
			logged_scores.push_back(entry.second);

		std::istringstream printed(capture.sink.str());
		while (std::getline(printed, line))
		{
			std::size_t at = line.find("error value: ");
			if (at != std::string::npos)
				printed_scores.push_back(line.substr(at + 13));
		}

		std::sort(logged_scores.begin(), logged_scores.end());
		std::sort(printed_scores.begin(), printed_scores.end());
		CHECK(logged_scores.size() == 5 && logged_scores == printed_scores);
	}

	// a NaN reference makes every tree invalid: the generation gives up instead of looping forever
	double unknown = std::nan("");
	CData broken({{"x", 1.0}}, unknown);   // the result may be an lvalue
	CClassifier stuck;
	stuck.s_nb_elm_per_gen(2);
	stuck.s_data(&broken);
	stuck.compute_new_generation(&generator);
	CHECK(stuck.get_tree_for_specific_generation(0, 0) == nullptr);
}


// both NaN, or equal up to rounding
static bool same_value(double a, double b)
{
	if (std::isnan(a) || std::isnan(b))
		return std::isnan(a) && std::isnan(b);

	if (std::isinf(a) || std::isinf(b))
		return a == b;

	return std::fabs(a - b) <= 1e-9 * std::max(1.0, std::max(std::fabs(a), std::fabs(b)));
}

static std::set<Node*> node_set(const Tree& tree)
{
	return std::set<Node*>(tree.nodes().begin(), tree.nodes().end());
}

static bool linked(const Tree& tree)
{
	for (Node* node : tree.nodes())
	{
		if ((node->left() != nullptr && node->left()->parent() != node) ||
			(node->right() != nullptr && node->right()->parent() != node) ||
			(node != tree.root() && node->parent() == nullptr))
			return false;
	}

	return true;
}

static bool follows_rules(const Tree& tree, const Rules& rules)
{
	if (tree.depth() > rules.max_depth || !linked(tree))
		return false;

	for (Node* node : tree.nodes())
	{
		if (node->type() == NodeType::OPERATOR && static_cast<tNode<OPERATOR>*>(node)->data() == OPERATOR::POW)
		{
			Node* exponent = node->right();
			if (exponent == nullptr || exponent->type() != NodeType::CONSTANT)
				return false;

			double order = static_cast<tNode<double>*>(exponent)->data();
			if (order != std::round(order) || order < 1 || order > rules.max_polynomial_order)
				return false;
		}
	}

	return true;
}

static void test_equation_text()
{
	// written in the parser's syntax, with the parentheses the tree's shape needs
	const char* equations[] = {"x^2-y+3", "x-(y-z)", "x/(y*2)", "x*y/2", "2^3^2", "(2^3)^2", "(x+1)*2",
							   "_exp[x]*_min[x,2]", "_sqrt[_cos[x]+1]^2", "x*(1.5e-05)", "x-y-z"};

	for (const char* equation : equations)
	{
		CParser parser;
		Tree tree;
		parser.build_tree_from_equation(&tree, equation);
		CHECK(tree.to_equation() == equation);
	}

	Tree negative;
	negative.makeRoot(new tNode<OPERATOR>(OPERATOR::PROD, constant(-2.5), variable("x")));
	negative.build();
	CHECK(negative.to_equation() == "(-2.5)*x");
	CHECK(Tree().to_equation() == "");

	// any generated tree reads back into a tree with the same values
	CTreeGenerator generator(Rules{5, 3, {"x", "y"}, -10, 10});

	for (int i = 0; i < 300; ++i)
	{
		Tree* tree = generator.generate_tree();
		CParser parser;
		Tree copy;
		parser.build_tree_from_equation(&copy, tree->to_equation(17));

		CHECK(!copy.isEmpty());
		CHECK(copy.to_equation(17) == tree->to_equation(17));

		for (double x : {-1.5, 0.5, 2.0})
		{
			Vars vars{{"x", x}, {"y", 1 - x}};
			CHECK(same_value(copy.compute_value(vars), tree->compute_value(vars)));
		}

		delete tree;
	}
}

static void test_tree_clone()
{
	CParser parser;
	Tree tree;
	parser.build_tree_from_equation(&tree, "_max[x,2]*(y-1)");

	Tree* copy = tree.clone();
	CHECK(copy->to_equation() == tree.to_equation());
	CHECK(copy->depth() == tree.depth());

	std::set<Node*> own = node_set(tree);
	for (Node* node : copy->nodes())
		CHECK(own.count(node) == 0);

	parser.build_tree_from_equation(&tree, "0");
	CHECK_NEAR(copy->compute_value({{"x", 3}, {"y", 2}}), 3);
	delete copy;

	Tree empty;
	Tree* empty_copy = empty.clone();
	CHECK(empty_copy->isEmpty());
	delete empty_copy;
}

static void test_variation_operators()
{
	Rules rules{5, 3, {"x", "y", "z"}, -10, 10};
	CTreeGenerator generator(rules);
	int changed_by_mutation = 0, changed_by_crossover = 0;

	for (int i = 0; i < 300; ++i)
	{
		Tree* mother = generator.generate_tree();
		Tree* father = generator.generate_tree();
		std::string mother_text = mother->to_equation(17), father_text = father->to_equation(17);

		Tree* mutant = generator.mutate(*mother);
		Tree* child = generator.crossover(*mother, *father);

		// the children follow the rules, are new trees, and leave the parents untouched
		for (Tree* offspring : {mutant, child})
		{
			CHECK(follows_rules(*offspring, rules));

			std::set<Node*> parents = node_set(*mother);
			std::set<Node*> others = node_set(*father);
			parents.insert(others.begin(), others.end());

			for (Node* node : offspring->nodes())
				CHECK(parents.count(node) == 0);
		}

		CHECK(mother->to_equation(17) == mother_text && father->to_equation(17) == father_text);

		changed_by_mutation += (mutant->to_equation(17) != mother_text);
		changed_by_crossover += (child->to_equation(17) != mother_text);

		delete mutant;
		delete child;
		delete mother;
		delete father;
	}

	CHECK(changed_by_mutation > 150);
	CHECK(changed_by_crossover > 150);

	// a subtree drawn for a deep position stays within max_depth
	Node* leaf = generator.generate_subtree(rules.max_depth);
	CHECK(leaf->left() == nullptr && leaf->right() == nullptr);
	delete leaf;
}

// best formula of each generation of a run seeded with 'seed'
static std::vector<std::string> seeded_run(unsigned int seed, int generations, bool with_test_samples = false)
{
	CData s1({{"x", 0.0}}, 1), s2({{"x", 1.0}}, 3), s3({{"x", 2.0}}, 5);
	CData t1({{"x", 5.0}}, 11), t2({{"x", -3.0}}, -5);
	CTreeGenerator generator(Rules{4, 2, {"x"}, -5, 5});
	generator.s_seed(seed);

	CClassifier classifier;
	classifier.s_seed(seed);
	classifier.s_nb_elm_per_gen(30);
	for (CData* sample : {&s1, &s2, &s3})
		classifier.s_data(sample);

	if (with_test_samples)
		for (CData* sample : {&t1, &t2})
			classifier.s_test_data(sample);

	std::vector<std::string> best;
	for (int g = 0; g < generations; ++g)
	{
		classifier.compute_new_generation(&generator);
		best.push_back(classifier.get_tree_for_specific_generation(0, g)->to_equation(17));
	}

	return best;
}

static void test_seeds()
{
	// the same seed gives the same trees (on a given standard library)
	CTreeGenerator a(Rules{5, 3, {"x", "y"}, -10, 10});
	CTreeGenerator b(Rules{5, 3, {"x", "y"}, -10, 10});
	a.s_seed(42);
	b.s_seed(42);

	for (int i = 0; i < 20; ++i)
	{
		Tree* ta = a.generate_tree();
		Tree* tb = b.generate_tree();
		Tree* ma = a.mutate(*ta);
		Tree* mb = b.mutate(*tb);
		CHECK(ta->to_equation(17) == tb->to_equation(17));
		CHECK(ma->to_equation(17) == mb->to_equation(17));
		delete ta; delete tb; delete ma; delete mb;
	}

	CHECK(seeded_run(7, 8) == seeded_run(7, 8));
	CHECK(seeded_run(7, 8) != seeded_run(8, 8));
}

// root mean squared error of a tree on samples, computed by hand
static double manual_rmse(Tree* tree, const std::vector<CData*>& samples)
{
	double sum = 0;

	for (CData* sample : samples)
	{
		double error = sample->get_result() - tree->compute_value(sample->get_data());
		sum += error * error;
	}

	return std::sqrt(sum / samples.size());
}

static void test_held_back_samples()
{
	// held-back samples never influence the evolution: same seed, same formulas
	for (unsigned int seed : {3u, 11u})
		CHECK(seeded_run(seed, 10, true) == seeded_run(seed, 10, false));

	// training and test errors are root mean squared errors
	std::vector<std::unique_ptr<CData>> data;
	std::vector<CData*> train, test;
	for (double x = -3; x <= 3; x += 0.5)
	{
		data.emplace_back(new CData({{"x", x}}, x*x + 1));
		(data.size() % 4 == 0 ? test : train).push_back(data.back().get());
	}

	CTreeGenerator generator(Rules{4, 2, {"x"}, -5, 5});
	CClassifier classifier;
	classifier.s_nb_elm_per_gen(40);
	classifier.s_nb_gen(5);
	for (CData* sample : train)
		classifier.s_data(sample);

	classifier.evolve(&generator);
	CHECK(std::isnan(classifier.get_test_score(0, 4)));            // no test sample yet

	for (CData* sample : test)
		classifier.s_test_data(sample);

	for (int t = 0; t < 40; t += 13)
	{
		Tree* tree = classifier.get_tree_for_specific_generation(t, 4);
		CHECK_NEAR(classifier.get_score(t, 4), manual_rmse(tree, train));

		double expected = manual_rmse(tree, test);
		double actual = classifier.get_test_score(t, 4);
		CHECK(std::isfinite(expected) ? std::fabs(actual - expected) <= 1e-9 * std::max(1.0, expected) : std::isnan(actual));
	}

	CHECK(std::isnan(classifier.get_test_score(40, 4)) && std::isnan(classifier.get_test_score(0, 5)));
}

static void test_evolution()
{
	// samples of x^2 - y + 3 on a grid
	std::vector<std::unique_ptr<CData>> samples;
	for (double x = -2; x <= 2; x += 1)
		for (double y = -2; y <= 2; y += 1)
			samples.emplace_back(new CData({{"x", x}, {"y", y}}, x*x - y + 3));

	bool improved = false;

	for (unsigned int seed : {1u, 2u, 3u})
	{
		CTreeGenerator generator(Rules{4, 2, {"x", "y"}, -5, 5});
		generator.s_seed(seed);

		CClassifier classifier;
		classifier.s_seed(seed);
		classifier.s_nb_elm_per_gen(60);
		classifier.s_nb_gen(15);
		for (auto& sample : samples)
			classifier.s_data(sample.get());

		classifier.evolve(&generator);
		CHECK(classifier.nb_generations() == 15);

		std::set<Tree*> trees;
		std::set<Node*> nodes;
		std::size_t node_count = 0;

		for (int g = 0; g < classifier.nb_generations(); ++g)
		{
			// a full generation, sorted best first
			CHECK(classifier.get_tree_for_specific_generation(59, g) != nullptr);
			CHECK(classifier.get_tree_for_specific_generation(60, g) == nullptr);

			for (int t = 1; t < 60; ++t)
				CHECK(classifier.get_score(t-1, g) <= classifier.get_score(t, g));

			// elitism: the best score never gets worse
			if (g > 0)
				CHECK(classifier.get_score(0, g) <= classifier.get_score(0, g-1));

			for (int t = 0; t < 60; ++t)
			{
				Tree* tree = classifier.get_tree_for_specific_generation(t, g);
				trees.insert(tree);
				node_count += tree->nodes().size();
				nodes.insert(tree->nodes().begin(), tree->nodes().end());

				CHECK(follows_rules(*tree, generator.rules()));
			}
		}

		// every generation owns its own trees and nodes
		CHECK(trees.size() == 15 * 60u);
		CHECK(nodes.size() == node_count);

		improved |= classifier.get_score(0, 14) < classifier.get_score(0, 0);
	}

	CHECK(improved);

	// small populations and an elite larger than the population
	CTreeGenerator generator(Rules{3, 2, {"x"}, -5, 5});
	CClassifier tiny;
	tiny.s_nb_elm_per_gen(1);
	tiny.s_nb_elite(3);
	tiny.s_nb_gen(5);
	tiny.s_data(samples[0].get());
	tiny.evolve(&generator);
	CHECK(tiny.get_tree_for_specific_generation(0, 4) != nullptr);
	CHECK(tiny.get_tree_for_specific_generation(1, 4) == nullptr);
	CHECK(std::isnan(tiny.get_score(1, 4)) && std::isnan(tiny.get_score(0, 5)));

	// nothing valid: every generation is empty, drawn at random again each time
	CData broken({{"x", 1.0}}, std::nan(""));
	CClassifier stuck;
	stuck.s_nb_elm_per_gen(1);
	stuck.s_nb_gen(2);
	stuck.s_data(&broken);
	stuck.evolve(&generator);
	CHECK(stuck.nb_generations() == 2 && stuck.get_tree_for_specific_generation(0, 1) == nullptr);
}

int main()
{
	std::vector<std::pair<const char*, std::function<void()>>> tests = {
		{"precedence and associativity", test_precedence_and_associativity},
		{"decimals ignore the C locale", test_decimals_ignore_the_c_locale},
		{"unary minus", test_unary_minus},
		{"functions", test_functions},
		{"malformed equations are rejected", test_malformed_equations_are_rejected},
		{"tree reuse resets state", test_tree_reuse_resets_state},
		{"missing variable is NaN", test_missing_variable_is_nan},
		{"invalid values propagate", test_invalid_values_propagate},
		{"data", test_data},
		{"queries", test_queries},
		{"search by node type", test_search_by_node_type},
		{"operator constructor links children", test_operator_constructor_links_children},
		{"replace", test_replace},
		{"remove_intermediate", test_remove_intermediate},
		{"expand_from_root", test_expand_from_root},
		{"heap positions", test_positions},
		{"clone is deep", test_clone_is_deep},
		{"build is linear", test_build_is_linear},
		{"generator rules", test_generator_rules},
		{"classifier", test_classifier},
		{"equation text", test_equation_text},
		{"tree clone", test_tree_clone},
		{"variation operators", test_variation_operators},
		{"seeds", test_seeds},
		{"held-back samples", test_held_back_samples},
		{"evolution", test_evolution},
	};

	for (auto& test : tests)
	{
		int before = g_failures;
		test.second();
		std::cout << (g_failures == before ? "[ OK ] " : "[FAIL] ") << test.first << std::endl;
	}

	std::cout << g_checks << " checks, " << g_failures << " failures" << std::endl;

	return g_failures == 0 ? 0 : 1;
}
