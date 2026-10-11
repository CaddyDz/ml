#include <QApplication>
#include <QGraphicsView>
#include "widget.h"
#include "tree.hpp"
#include "render.hpp"
#include "parser.hpp"
#include "treegenerator.hpp"
#include "classifier.hpp"

#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <limits>
#include <memory>
#include <vector>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	//---------------------------------------------
	// Create a Tree from scratch
	//---------------------------------------------
	Tree tree0;

	Node* sum_l1 = new tNode<double>(4.0);
	sum_l1->s_type(NodeType::CONSTANT);

	// This is the only minor drawback : we have to declare explicitely the type of each node, it can not be deduced at compilation time by the template class tNode
	// We could use typeid to declare it in the tNode constructor, but with this version of gcc, results are quite unexpected ans should not be portable

	Node* sum_r1 = new tNode<std::string>("y");
	sum_r1->s_type(NodeType::VARIABLE);

	Node* sum_l2 = new tNode<double>(2.0);
	sum_l2->s_type(NodeType::CONSTANT);

	Node* sum_r2 = new tNode<std::string>("x");
	sum_r2->s_type(NodeType::VARIABLE);

	Node* op1 = new tNode<OPERATOR>(OPERATOR::ADD,sum_l1,sum_r1);
	Node* op2 = new tNode<OPERATOR>(OPERATOR::ADD,sum_l2,sum_r2);
	Node* op0 = new tNode<OPERATOR>(OPERATOR::PROD,op1,op2);
	tree0.makeRoot(op0);

	tree0.build(); // Mandatory -- it builds several member structures to ease the access to the nodes

	//---------------------------------------------
	// Drawing the tree
	//---------------------------------------------

	CRender render0;
	render0.drawTree(&tree0);
	QGraphicsView view0(render0.scene());
	view0.show();

	//---------------------------------------------
	// Parsing an equation
	//---------------------------------------------
	CParser parser;

	Tree tree1;
	std::string equation1 = "3+4*2/(1-5)^2^3-6*z";
	parser.build_tree_from_equation(&tree1, equation1);

	CRender render1;
	render1.drawTree(&tree1);
	QGraphicsView view1(render1.scene());
	view1.show();

	//---------------------------------------------
	// Parse a simple function
	//---------------------------------------------
	Tree tree2;
	std::string function = "_exp[3+x]";
	parser.build_tree_from_equation(&tree2, function);

	CRender render2;
	render2.drawTree(&tree2);

	QGraphicsView view2(render2.scene());
	view2.show();

	//---------------------------------------------
	// Parse a complex equation
	//---------------------------------------------
	Tree tree3;
	std::string cfunction = "3+4*_exp[3+x]/(1-5)^2^3-6*z";
	parser.build_tree_from_equation(&tree3, cfunction);
	std::cout << "TREE DEPTH: " << tree3.depth() << std::endl;

	CRender render3;
	render3.drawTree(&tree3);

	QGraphicsView view3(render3.scene());
	view3.show();

	//---------------------------------------------
	// global search
	//---------------------------------------------

	Node* sd = tree3.find_node_from_val("4");
	if (sd)
		std::cout << "Node has been found at depth "<<sd->depth()<<std::endl;
	else
		std::cout << "Node not found " << std::endl;

	//---------------------------------------------
	// search at given depth
	//---------------------------------------------

	sd = tree3.find_node_from_val("/");
	if (sd) sd = tree3.find_node_from_val_at_depth("/",sd->depth());

	if (sd)
		std::cout << "Node has been found at depth "<<sd->depth()<<std::endl;
	else
		std::cout << "Node not found " << std::endl;

	//---------------------------------------------
	// simple mutation
	//---------------------------------------------

	Tree tree4;
	parser.build_tree_from_equation(&tree4, "(x+1)^2");

	// print the tree
	CRender render40;
	render40.drawTree(&tree4);

	QGraphicsView view40(render40.scene());
	view40.show();

	Node* mutate = tree4.find_node_from_val("^");
	if (mutate)
	{
		tNode<OPERATOR>* tmutate = dynamic_cast<tNode<OPERATOR>*>(mutate);
		tmutate->s_data(OPERATOR::PROD);
		mutate = tree4.find_node_from_val_at_depth("*",tmutate->depth());
		if (mutate)
			std::cout << "Node has been found at depth "<<mutate->depth()<<std::endl;
		else
			std::cout << "Node not found " << std::endl;
	}
	else
	  std::cout << "Simple Mutation - Node not found " << std::endl;

	CRender render41;
	render41.drawTree(&tree4);
	QGraphicsView view41(render41.scene());
	view41.show();

	//---------------------------------------------
	// Expanding tree from root
	//---------------------------------------------
	Tree tree5;
	parser.build_tree_from_equation(&tree5, "x+y");

	Node* op = new tNode<FUNCTION>(FUNCTION::COS);
	op->s_type(NodeType::FUNCTION);
	tree5.expand_from_root(op);

	CRender render5;
	render5.drawTree(&tree5);
	QGraphicsView view5(render5.scene());
	view5.show();

	//---------------------------------------------
	// Crossing over
	//---------------------------------------------

	Tree tree6;
	parser.build_tree_from_equation(&tree6, "(3*(x+1))^2");

	CRender render6;
	render6.drawTree(&tree6);
	QGraphicsView view6(render6.scene());
	view6.show();

	Tree tree7;
	parser.build_tree_from_equation(&tree7, "_sqrt[x]");

	Node* gene = tree6.find_node_from_val_at_depth("+",3);

	if (gene)
	{
		// graft a copy: tree7 keeps owning (and will delete) its own nodes
		tree6.replace(gene,tree7.root()->clone());
	}

	CRender render7;
	render7.drawTree(&tree6);
	QGraphicsView view7(render7.scene());
	view7.show();

	//---------------------------------------------
	// Remove intermediate node
	//---------------------------------------------
	Tree tree8;
	parser.build_tree_from_equation(&tree8, "(1+_cos[2*x])^2");

	Node* node_to_remove = tree8.find_node_from_val("cos");

	if (node_to_remove)
		tree8.remove_intermediate(node_to_remove);

	CRender render8;
	render8.drawTree(&tree8);
	QGraphicsView view8(render8.scene());
	view8.show();

	//---------------------------------------------
	// Print list of variables
	//---------------------------------------------
	Tree tree9;
	parser.build_tree_from_equation(&tree9, "3+4*((y+2*x+t)^2-z^3+_sin[theta])+(7*z+y^2)");

	tree9.print_variables();

	//---------------------------------------------
	// Eval function value
	//---------------------------------------------
	std::map<std::string, double> vars{ {"x",-5},{"y",2},{"z",1},{"t",-1},{"theta",M_PI/4}};
	std::cout << "Value with parameters x=-5, y=2, z=1, t=-1, theta=PI/4 is: " << tree9.compute_value(vars) << std::endl;

	std::map<std::string, double> vars1{ {"x",6},{"y",-4},{"z",10},{"t",-1},{"theta",M_PI/6}};
	std::cout << "Value with parameters x=6, y=-4, z=10, t=-1, theta=PI/6 is: " << tree9.compute_value(vars1) << std::endl;

	std::map<std::string, double> vars2{ {"x",-3},{"y",1},{"z",-8},{"t",5},{"theta",M_PI/2}};
	std::cout << "Value with parameters x=-3, y=1, z=-8, t=5, theta=PI/2 is: " << tree9.compute_value(vars2) << std::endl;

	//---------------------------------------------
	// Generate Tree
	//---------------------------------------------
	Rules r1 {5, 5, {"x", "y"}, 0, 100000};

	CTreeGenerator tg1(r1);
	std::unique_ptr<Tree> tree10(tg1.generate_tree());

	CRender render10;
	render10.drawTree(tree10.get());
	QGraphicsView view10(render10.scene());
	view10.show();

	//---------------------------------------------
	// Symbolic regression: evolve a formula that fits a set of data
	//---------------------------------------------

	// the seed of the run, from the command line to replay it (./Tree 4)
	unsigned int seed = static_cast<unsigned int>(std::time(nullptr));

	if (argc > 1)
	{
		char* end = nullptr;
		unsigned long value = std::strtoul(argv[1], &end, 10);

		if (end != argv[1] && *end == '\0')
			seed = static_cast<unsigned int>(value);
	}

	// samples of the function to find, x^2 - y + 3, on a 7x7 grid, with a little
	// noise on each result like measured data would have: on exact results a
	// formula off by a constant gets the same error on any set of points
	std::mt19937 sampling(seed);
	std::normal_distribution<double> noise(0.0, 0.1);
	std::vector<std::unique_ptr<CData>> samples;
	for (double x = -3; x <= 3; x += 1)
		for (double y = -3; y <= 3; y += 1)
			samples.emplace_back(new CData({{"x",x},{"y",y}}, x*x - y + 3 + noise(sampling)));

	// a quarter of them, drawn at random, is held back to test the result: the
	// evolution never sees them
	std::shuffle(samples.begin(), samples.end(), sampling);
	const std::size_t nb_test = samples.size() / 4;

	CClassifier cl;
	cl.s_nb_elm_per_gen(200);
	for (std::size_t i = 0; i < samples.size(); ++i)
	{
		if (i < nb_test)
			cl.s_test_data(samples[i].get());
		else
			cl.s_data(samples[i].get());
	}

	Rules r2 {4, 2, {"x", "y"}, -5, 5};
	CTreeGenerator tg2(r2);
	tg2.s_seed(seed);
	cl.s_seed(seed);

	std::cout << "Evolving a formula for x^2-y+3 from " << samples.size() - nb_test << " noisy samples, "
			  << nb_test << " others held back for testing (seed " << seed << ")" << std::endl;

	const int generations = 40;
	double best = std::numeric_limits<double>::infinity();

	for (int i = 0; i < generations; ++i)
	{
		cl.compute_new_generation(&tg2);

		if (cl.get_score(0, i) < best)
		{
			best = cl.get_score(0, i);
			std::cout << "Generation " << i+1 << ": training error " << best
					  << ", test error " << cl.get_test_score(0, i) << "  "
					  << cl.get_tree_for_specific_generation(0, i)->to_equation() << std::endl;
		}
	}

	Tree* fittest = cl.get_tree_for_specific_generation(0, generations-1);

	CRender render11;
	render11.drawTree(fittest);
	QGraphicsView view11(render11.scene());
	if (fittest)
		view11.setWindowTitle(QString::fromStdString("Best formula: " + fittest->to_equation()));
	view11.show();

	return a.exec();
}
