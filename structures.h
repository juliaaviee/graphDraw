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


class Node;

class Edge : public QGraphicsLineItem {
public:
    Edge(Node* sourceNode, Node* destNode);
    Node* getSource();
    Node* getDest();
    void updatePos();
    void highlightEdge();
    void clearEdge();
private:
    Node* source;
    Node* dest;
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
