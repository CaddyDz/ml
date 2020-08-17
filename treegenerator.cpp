#include "treegenerator.hpp"

CTreeGenerator::CTreeGenerator(const Rules& rules)
{
    m_rules = rules;
    std::srand(std::time(nullptr));
}

void CTreeGenerator::generate_node(Node*& node, unsigned int depth) const
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(m_rules.min_number, m_rules.max_number);
    Node* left, *right;

    NodeType type;

    if (depth == 1)
        type = NodeType::OPERATOR;
    else if (depth == m_rules.max_depth)
        type = static_cast<NodeType>(std::rand()%2);
    else
        type = static_cast<NodeType>(std::rand()%4);

    switch(type)
    {
    case NodeType::CONSTANT:
    {
        double val = distribution(generator);
        node = new tNode<double>(val);
        node->s_type(NodeType::CONSTANT);
        node->s_depth(depth);
        break;
    }

    case NodeType::VARIABLE:
    {
        int ivar = std::rand() % (m_rules.m_list_variables.size());

        node = new tNode<std::string>(m_rules.m_list_variables[ivar]);
        node->s_type(NodeType::VARIABLE);
        node->s_depth(depth);

        break;
    }

    case NodeType::FUNCTION:
    {
        FUNCTION fn = static_cast<FUNCTION>(std::rand() % 8);

        node = new tNode<FUNCTION>(fn);
        node->s_type(NodeType::FUNCTION);
        node->s_depth(depth++);

        if (fn == FUNCTION::MIN || fn == FUNCTION::MAX)
        {
            this->generate_node(left,depth);
            this->generate_node(right,depth);
            node->s_children(left,right);
        }
        else
        {
            this->generate_node(left,depth);
            node->s_left(left);
        }

        break;
    }

    case NodeType::OPERATOR:
    {
        OPERATOR op = static_cast<OPERATOR>(std::rand() % 5);

        node = new tNode<OPERATOR>(op);
        node->s_type(NodeType::OPERATOR);
        node->s_depth(depth++);

        this->generate_node(left,depth);
        this->generate_node(right,depth);
        node->s_children(left,right);

        break;
    }

    default:
        break;
    }

    return;
}

Tree* CTreeGenerator::generate_tree() const
{
    Tree* generated_tree = new Tree();

    Node* root;
    this->generate_node(root,1);

    generated_tree->makeRoot(root);
    generated_tree->build();

    // check the tree afterwards
    this->check_rules(generated_tree);

    return generated_tree;
}

bool CTreeGenerator::check_rules(Tree* tree) const
{
    if (!tree)
    {
        std::cerr<<"Tree is empty"<<std::endl;
        return false;
    }
    else if( tree->depth() > m_rules.max_depth)
    {
        std::cerr<<"Tree max depth exceeded"<<std::endl;
        return false;
    }

    std::vector<Node*> nodes = tree->nodes();

    // Remove obvious subtrees, for instance */0, *^infinity, *^0,
    // TODO exp(ln(.)), sqrt(x^2), sqrt(x)^2

    std::for_each(nodes.begin(), nodes.end(), [this, &tree](Node* node)
    {
        if ((node != nullptr) && tree->find_node(node)) // must ensure that node has not been destroyed by a previous check
        {
            if (node->type() == NodeType::OPERATOR)
            {
                tNode<OPERATOR>* op = static_cast<tNode<OPERATOR>*>(node);
                Node* right = node->right();
                NodeType type = right->type();

                // Check for max polynomial order
                if (op->data() == OPERATOR::POW)
                {
                    /*
                     * Two possible cases :
                     *    * right is a number, we must ensure this number is smaller than max_polynomial_order
                     *    * right is anything else (exponent can be very high, implying an overflow), we replace it by a number which follows the previous rules
                     */

                    if (type == NodeType::CONSTANT)
                    {

                        tNode<double>* exponent = static_cast<tNode<double>*>(right);
                        if (exponent->data() > this->m_rules.max_polynomial_order || exponent->data() == 0)
                            exponent->s_data(this->m_rules.max_polynomial_order);
                    }
                    else
                    {
                        int exp = rand()%(this->m_rules.max_polynomial_order-1)+1;
                        tNode<double>* exponent = new tNode<double>(exp);
                        exponent->s_type(NodeType::CONSTANT);

                        // on mute le noeud
                        tree->replace(right,exponent);

                    }
                }
                else if (op->data() == OPERATOR::DIV)
                {
                    Node* right = node->right();
                    NodeType type = right->type();

                    if (type == NodeType::CONSTANT)
                    {
                        tNode<double>* div = static_cast<tNode<double>*>(right);

                        if (div->data() == 0.0)
                        {
                            std::random_device rd;
                            std::mt19937 generator(rd());
                            std::uniform_real_distribution<double> distribution(m_rules.min_number, m_rules.max_number);

                            double val = distribution(generator);

                            div->s_data(val);

                        }
                    }
                }
            }
        }
    }
    );

    return true;
}



