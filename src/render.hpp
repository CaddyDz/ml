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

#include <math.h>
#include <string.h>
#include "tree.hpp"

using namespace std;

class CRender
{
private:
    QGraphicsScene* m_scene;

public:
    CRender();
    ~CRender();

    QGraphicsScene* scene() const{return m_scene;};
    void drawRect(qreal x, qreal y, qreal rad, string txt);
    void drawCircle(qreal x, qreal y, qreal rad, string txt);
    void drawLine(qreal x1, qreal y1, qreal x2, qreal y2);
    void drawTree(Tree*);

    void drawNode(Node*,qreal x, qreal y);

    void clear();
};

#endif // RENDER_HPP
