#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QFont>
#include <QRectF>
#include <QPen>
#include <QPainter>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QPointF>
#include <QPolygonF>

class Node;

class Edge : public QObject, public QGraphicsLineItem {
    Q_OBJECT
public:
    Edge(Node* sourceNode, Node* destNode, int w=1, int n=0);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    Node* getSource();
    Node* getDest();
    void updatePos();
    void highlightEdge();
    void clearEdge();
    void setWeight(int w);
    void changeNudge(int n);
    int getWeight();
signals:
    void selected(Edge* e);
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
private:
    QGraphicsTextItem* label;
    Node* source;
    Node* dest;
    int weight;
    int nudge; // The nudge factor is what helps us make the edges parallel to each other, in case it's a 2-way connection
};

struct Route {
    QString visual;
    QList<Edge*> edges;
    int totalCost=0;
};

class Node : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    explicit Node(const QString& label, QGraphicsItem* parent = nullptr);
    void setLabel(const QString& newLabel);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    QString getLabel();
    QList<Edge*> getCons();
    void addEdge(Edge* e);
    void removeEdge(Edge* e);
signals:
    void selected(Node* node);
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    QGraphicsTextItem* labelItem;
    QList<Edge*> cons;
};
#endif // STRUCTURES_H
