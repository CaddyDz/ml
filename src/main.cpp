#include <QApplication>
#include <QGraphicsView>
#include "widget.h"
#include "tree.hpp"
#include "render.hpp"
#include "parser.hpp"
#include "treegenerator.hpp"
#include "classifier.hpp"

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
        tree6.replace(gene,tree7.root());
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
    std::cout << "Value with parameters x=3, y=2, z=1, t=-1, theta=PI/4 is: " << tree9.compute_value(vars) << std::endl;

    std::map<std::string, double> vars1{ {"x",6},{"y",-4},{"z",10},{"t",-1},{"theta",M_PI/6}};
    std::cout << "Value with parameters x=6, y=-4, z=10, t=-1, theta=PI/6 is: " << tree9.compute_value(vars1) << std::endl;

    std::map<std::string, double> vars2{ {"x",-3},{"y",1},{"z",-8},{"t",5},{"theta",M_PI/2}};
    std::cout << "Value with parameters x=-3, y=1, z=-8, t=5, theta=PI/2 is: " << tree9.compute_value(vars2) << std::endl;

    //---------------------------------------------
    // Generate Tree
    //---------------------------------------------
    Rules r1 {5, 5, {"x", "y"}, 0, 100000};

    CTreeGenerator tg1(r1);
    Tree* tree10 = tg1.generate_tree();

    CRender render10;
    render10.drawTree(tree10);
    QGraphicsView view10(render10.scene());
    view10.show();

    //---------------------------------------------
    // Generate N tree and compute the errors against a set of data
    //---------------------------------------------

    // function to fit is x^2 - y + 3
    CData* data1 = new CData({{"x",0.0},{"y",-3.0}},6);
    CData* data2 = new CData({{"x",1.0},{"y",-1.0}},5);
    CData* data3 = new CData({{"x",2.0},{"y",3.0}},4);
    CData* data4 = new CData({{"x",3.0},{"y",9.0}},3);
    CData* data5 = new CData({{"x",4.0},{"y",17.0}},3);

    CClassifier cl;
    cl.s_nb_elm_per_gen(20);
    cl.s_data(data1);
    cl.s_data(data2);
    cl.s_data(data3);
    cl.s_data(data4);
    cl.s_data(data5);

    Rules r2 {4, 2, {"x", "y"}, -50, 50};

    CTreeGenerator tg2(r2);

    for(int i=0; i<1; ++i)
    {
        std::cout << "Generation --> "<<i+1<<"\n";
        cl.compute_new_generation(&tg2);
        cl.print_generation(i);

        //Tree* tree11 = cl.get_tree_for_specific_generation(i,0);

       /* CRender render11;
        render11.drawTree(tree11);
        QGraphicsView view11(render11.scene());
        view11.show()*/
    }

    if (data1 != nullptr) delete data1;
    if (data2 != nullptr) delete data2;
    if (data3 != nullptr) delete data3;
    if (data4 != nullptr) delete data4;
    if (data5 != nullptr) delete data5;

    return a.exec();
}
