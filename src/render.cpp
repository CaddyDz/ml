#include "render.hpp"

#include <algorithm>
#include <cmath>

CRender::CRender()
:m_scene(new QGraphicsScene())
{
	m_scene->setSceneRect(0,0,1500,500);

	// black drawing on a white background, whatever the palette (a dark theme would
	// otherwise draw white labels and black shapes on a dark background)
	m_scene->setBackgroundBrush(Qt::white);
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

namespace
{
	struct Slot
	{
		Node* node;
		int level;
	};

	// In-order sequence of the nodes: a node comes after all the nodes of its left
	// subtree and before all the nodes of its right subtree. Giving each node its own
	// column in this order, and its level as row, no two nodes can overlap.
	void in_order(Node* node, int level, std::vector<Slot>& sequence)
	{
		if (node == nullptr)
			return;

		in_order(node->left(), level+1, sequence);
		sequence.push_back({node, level});
		in_order(node->right(), level+1, sequence);
	}

	std::string label(Node* node)
	{
		std::ostringstream os;

		switch (node->type())
		{
		case NodeType::OPERATOR:
			os << op2str[static_cast<tNode<OPERATOR>*>(node)->data()];
			break;

		case NodeType::VARIABLE:
			os << static_cast<tNode<std::string>*>(node)->data();
			break;

		case NodeType::CONSTANT:
			os << static_cast<tNode<double>*>(node)->data();
			break;

		case NodeType::FUNCTION:
			os << fn2str[static_cast<tNode<FUNCTION>*>(node)->data()];
			break;

		default:
			break;
		}

		return os.str();
	}

	// half width of a node shape: the node radius, or more if the label needs it. The
	// ellipse (x/rx)^2 + (y/ry)^2 <= 1, with ry = node_radius, must contain the corners
	// of the text, not only its width.
	qreal half_width(const std::string& txt)
	{
		QGraphicsTextItem text(txt.c_str());
		QFontMetricsF metrics(text.font());
		qreal half_text = metrics.horizontalAdvance(QString::fromStdString(txt))/2 + 2;
		qreal height_ratio = std::min<qreal>(metrics.height()/2/node_radius, 0.9);

		return std::max<qreal>(node_radius, half_text/std::sqrt(1 - height_ratio*height_ratio));
	}

	// Point where the segment leaving 'center' in direction 'dir' (unit vector) crosses
	// the border of a node shape of half sizes rx and ry: an ellipse, or the rectangle
	// drawn for the root.
	QPointF border(QPointF center, QPointF dir, qreal rx, qreal ry, bool rect)
	{
		qreal distance;

		if (rect)
		{
			qreal dx = std::abs(dir.x()), dy = std::abs(dir.y());
			distance = (dx*ry > dy*rx) ? rx/dx : ry/dy;
		}
		else
			distance = 1/std::sqrt((dir.x()/rx)*(dir.x()/rx) + (dir.y()/ry)*(dir.y()/ry));

		return center + distance*dir;
	}
}

void CRender::drawShape(QPointF center, qreal rx, qreal ry, std::string txt, bool rect)
{
	if (m_scene)
	{
		QRectF shape(center.x()-rx, center.y()-ry, 2*rx, 2*ry);

		if (rect)
			m_scene->addItem(new QGraphicsRectItem(shape));
		else
			m_scene->addItem(new QGraphicsEllipseItem(shape));

		QGraphicsTextItem* text = new QGraphicsTextItem(txt.c_str());
		text->setDefaultTextColor(Qt::black);
		QRectF bounds = text->boundingRect();
		text->setPos(center.x()-bounds.width()/2,center.y()-bounds.height()/2);
		m_scene->addItem(text);
	}
}

void CRender::drawRect(qreal x, qreal y, qreal rad, std::string txt)
{
	this->drawShape(QPointF(x+rad,y+rad), rad, rad, txt, true);
}

void CRender::drawCircle(qreal x, qreal y, qreal rad, std::string txt)
{
	this->drawShape(QPointF(x+rad,y+rad), rad, rad, txt, false);
}

void CRender::drawLine(qreal x1, qreal y1, qreal x2, qreal y2)
{
	if (m_scene)
	{
		QGraphicsLineItem* line = new QGraphicsLineItem(x1, y1, x2, y2);
		m_scene->addItem(line);
	}
}

void CRender::drawSubtree(Node* node, qreal x, qreal y, bool root_as_rect)
{
	// (x, y) is the top left corner of the drawing
	if (node == nullptr || m_scene == nullptr)
		return;

	std::vector<Slot> sequence;
	in_order(node, 0, sequence);

	// columns from left to right, each as wide as its node, with a constant gap
	const qreal gap = 2*shift_x - 2*node_radius;
	std::map<Node*,QPointF> centers;
	std::map<Node*,qreal> half_widths;
	qreal right_edge = x - gap;

	for (const Slot& slot: sequence)
	{
		qreal rx = half_width(label(slot.node));
		QPointF center(right_edge + gap + rx, y + node_radius + slot.level*shift_y);

		centers[slot.node] = center;
		half_widths[slot.node] = rx;
		right_edge = center.x() + rx;
	}

	for (const Slot& slot: sequence)
	{
		Node* current = slot.node;
		QPointF center = centers.at(current);
		qreal rx = half_widths.at(current);
		bool rect = root_as_rect && current == node;

		this->drawShape(center, rx, node_radius, label(current), rect);

		for (Node* child: {current->left(), current->right()})
		{
			if (child == nullptr)
				continue;

			QPointF child_center = centers.at(child);
			QPointF delta = child_center - center;
			QPointF dir = delta / std::sqrt(QPointF::dotProduct(delta, delta));

			QPointF from = border(center, dir, rx, node_radius, rect);
			QPointF to = border(child_center, -dir, half_widths.at(child), node_radius, false);

			this->drawLine(from.x(), from.y(), to.x(), to.y());
		}
	}

	// grow the scene so that the whole drawing can be scrolled to
	m_scene->setSceneRect(m_scene->sceneRect().united(m_scene->itemsBoundingRect().adjusted(-20,-20,20,20)));
}

void CRender::drawNode(Node* node, qreal x, qreal y)
{
	// draws the whole subtree of node, (x, y) being the top left corner of the drawing
	this->drawSubtree(node, x, y, false);
}

void CRender::drawTree(Tree* tree)
{
	if (m_scene == nullptr)
		return;

	// a new drawing replaces the previous one (e.g. after a mutation), even when
	// there is nothing to draw
	this->clear();

	if (tree == nullptr || tree->isEmpty() || tree->root() == nullptr)
	{
		m_scene->setSceneRect(0,0,1500,500);   // the size of a new scene
		return;
	}

	// the root is drawn as a rectangle, the other nodes as ellipses
	this->drawSubtree(tree->root(), 0, 0, true);

	// fit the scene to the drawing, whatever the size of the tree
	m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-20,-20,20,20));
}
