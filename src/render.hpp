#ifndef RENDER_HPP
#define RENDER_HPP

#define shift_x 25
#define shift_y 65
#define node_radius 20
#define _USE_MATH_DEFINES

#include <QPainter>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QPoint>
#include <QFontMetricsF>

#include <math.h>
#include <string.h>
#include <map>
#include <sstream>
#include <vector>
#include "tree.hpp"

using namespace std;

/**
 * Draws a Tree in a QGraphicsScene, which a QGraphicsView displays:
 *
 *     CRender render;
 *     render.drawTree(&tree);
 *     QGraphicsView view(render.scene());
 *     view.show();
 *
 * Each node gets its own column (in-order: left subtree, node, right subtree) as
 * wide as its label, and its depth gives the row, so nodes never overlap. The root
 * is drawn as a rectangle, the other nodes as ellipses (circles for short labels).
 * The drawing is black on white whatever the palette. The scene belongs to the
 * renderer.
 */
class CRender
{
private:
	QGraphicsScene* m_scene;

	void drawSubtree(Node*, qreal x, qreal y, bool root_as_rect);
	void drawShape(QPointF center, qreal rx, qreal ry, string txt, bool rect);

public:
	CRender();
	~CRender();

	// the renderer owns its scene
	CRender(const CRender&) = delete;
	CRender& operator=(const CRender&) = delete;

	QGraphicsScene* scene() const{return m_scene;};

	// Shapes with a centered label; (x, y) is the top left corner and rad half the size
	void drawRect(qreal x, qreal y, qreal rad, string txt);
	void drawCircle(qreal x, qreal y, qreal rad, string txt);
	void drawLine(qreal x1, qreal y1, qreal x2, qreal y2);

	/// Draws the whole tree in place of the previous drawing (an empty tree just
	/// clears it) and fits the scene to it.
	void drawTree(Tree*);

	/// Adds the drawing of the subtree of a node, (x, y) being its top left corner,
	/// and grows the scene to contain it.
	void drawNode(Node*,qreal x, qreal y);

	void clear();   ///< removes everything drawn so far
};

#endif // RENDER_HPP
