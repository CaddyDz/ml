#include "render.hpp"

CRender::CRender()
:m_scene(new QGraphicsScene())
{
	m_scene->setSceneRect(0,0,1500,500);
}

CRender::~CRender()
{
	this->clear();
	if (m_scene) delete m_scene;
}

void CRender::clear()
{
	QList<QGraphicsItem*> items = m_scene->items();

	std::for_each(items.begin(), items.end(), [this](QGraphicsItem* item)
										  {
											this->m_scene->removeItem(item);
											delete item;
										  }
										);

	return;
}

void CRender::drawRect(qreal x, qreal y, qreal rad, std::string txt)
{
	if (m_scene)
	{
		QRect rect(x, y, 2*rad, 2*rad);
		QGraphicsRectItem* op = new QGraphicsRectItem(rect);
		m_scene->addItem(op);

		QGraphicsTextItem* text = new QGraphicsTextItem(txt.c_str());
		text->setPos(x+rad/2,y+rad/2);
		m_scene->addItem(text);
	}
}

void CRender::drawCircle(qreal x, qreal y, qreal rad, std::string txt)
{
	if (m_scene)
	{

		QRect rect(x, y, 2*rad, 2*rad);
		QGraphicsEllipseItem* op = new QGraphicsEllipseItem(rect);
		m_scene->addItem(op);

		QGraphicsTextItem* text = new QGraphicsTextItem(txt.c_str());
		text->setPos(x+rad/2,y+rad/2);
		m_scene->addItem(text);
	}
}

void CRender::drawLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
	if (m_scene)
	{
		QGraphicsLineItem* line = new QGraphicsLineItem(x1, y1, x2, y2);
		m_scene->addItem(line);
	}
}

void CRender::drawNode(Node* node, qreal x, qreal y)
{
	if (node)
	{
		double theta = 0.75*M_PI;
		std::ostringstream os;
		NodeType nt = node->type();

		if (nt == NodeType::OPERATOR)
		{
			tNode<OPERATOR>* tnode = dynamic_cast<tNode<OPERATOR>*>(node);
			os << op2str[tnode->data()];
		}
		else if (nt == NodeType::VARIABLE)
		{
			tNode<std::string>* tnode = dynamic_cast<tNode<std::string>*>(node);
			os << tnode->data();
		}
		else if (nt == NodeType::CONSTANT)
		{
			tNode<double>* tnode = dynamic_cast<tNode<double>*>(node);
			os << tnode->data();
		}
		else if (nt == NodeType::FUNCTION)
		{
			tNode<FUNCTION>* tnode = dynamic_cast<tNode<FUNCTION>*>(node);
			os << fn2str[tnode->data()];
		}

		this->drawCircle(x,y,node_radius,os.str());

		qreal y1 = y+shift_y;
		if (node->left())
		{
			qreal lx = x-1.5*shift_x;
			this->drawNode(node->left(),lx,y1);
			this->drawLine(x+node_radius*(1-sin(theta)),y+node_radius*(1-cos(theta)),lx+node_radius,y1);
		}

		if (node->right())
		{
			qreal rx = x+1.5*shift_x;
			this->drawNode(node->right(),rx,y1);
			this->drawLine(x+node_radius*(1+sin(theta)),y+node_radius*(1-cos(theta)),rx+node_radius,y1);
		}
	}

	return;
}

void CRender::drawTree(Tree* tree)
{
	if (tree->isEmpty())
			return;

	if (m_scene)
	{
		unsigned int depth = tree->depth();
		double theta = 0.25*M_PI;

		std::ostringstream os;

		// draw root - depth 1
		int x = 750;
		int y = 20;

		Node* root = tree->root();

		if (root)
		{
			NodeType nt = root->type();

			if (nt == NodeType::OPERATOR)
			{
				tNode<OPERATOR>* tnode = dynamic_cast<tNode<OPERATOR>*>(root);
				os << op2str[tnode->data()];
			}
			else if (nt == NodeType::VARIABLE)
			{
				tNode<std::string>* tnode = dynamic_cast<tNode<std::string>*>(root);
				os << tnode->data();
			}
			else if (nt == NodeType::CONSTANT)
			{
				tNode<double>* tnode = dynamic_cast<tNode<double>*>(root);
				os << tnode->data();
			}
			else if (nt == NodeType::FUNCTION)
			{
				tNode<FUNCTION>* tnode = dynamic_cast<tNode<FUNCTION>*>(root);
				os << fn2str[tnode->data()];
			}

			this->drawRect(x,y,node_radius, os.str());
		}

		qreal y1 = y+shift_y;

		// Draw others nodes
		if (root->left())
		{
			qreal lx = x-depth*shift_x;
			this->drawNode(root->left(),lx,y1);
			this->drawLine(x,y+2*node_radius,lx+node_radius*(1+sin(theta)),y1 + node_radius*(1-cos(theta)));
		}

		if (root->right())
		{
			qreal rx = x+depth*shift_x;
			this->drawNode(root->right(),rx,y1);
			this->drawLine(x+2*node_radius,y+2*node_radius,rx+node_radius*(1-sin(theta)),y1 + node_radius*(1-cos(theta)));
		}

	}
}
