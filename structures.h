#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QFont>
#include <QRectF>
#include <QPen>
#include <QPainter>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QLineEdit>
#include <QPointF>
#include <QPolygonF>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QSpinBox>
#include <QWheelEvent>

class Node;

class Edge : public QObject, public QGraphicsLineItem {
    Q_OBJECT
public:
    Edge(Node* sourceNode, Node* destNode, int w=1);
    Node* getSource();
    Node* getDest();
    void updatePos();
    void highlightEdge();
    void clearEdge();
    void setWeight(int w);
    int getWeight();
    void directionToggle(bool t);
    void setCounterpart(Edge* c);
    Edge* getCounterpart();
    QGraphicsTextItem* getLabel();
signals:
    void selected(Edge* e);
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
private:
    QGraphicsTextItem* label;
    Node* source;
    Node* dest;
    Edge* ctrpart=nullptr;
    int weight;
    bool directed=true;
};

struct Route {
    QString visual;
    QList<Edge*> edges;
    int totalCost=0;
};

class CSpinBox : public QSpinBox {
    Q_OBJECT
public:
    using QSpinBox::QSpinBox;

protected:
    void stepBy(int steps) override {
        QSpinBox::stepBy(steps);
        lineEdit()->deselect();
    }
};

class Node : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    explicit Node(const QString& label, QGraphicsItem* parent = nullptr);
    void setLabel(const QString& newLabel);
    QString getLabel();
    QList<Edge*> getCons();
    void addEdge(Edge* e);
    void removeEdge(Edge* e);
    QRectF boundingRect() const override;
signals:
    void selected(Node* node);
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
    QGraphicsTextItem* labelItem;
    QList<Edge*> cons;
};

class GraphView : public QGraphicsView {
    Q_OBJECT

public:
    GraphView(QWidget *parent = nullptr)
        : QGraphicsView(parent), zoomFactor(1.15) {}

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
            double factor = event->angleDelta().y() > 0 ? zoomFactor : 1.0 / zoomFactor;
            double newZoom = currentZoom * factor;

            if (newZoom < minZoom || newZoom > maxZoom) return;  // Block zooming if out of bounds
            scale(factor, factor);
            currentZoom = newZoom;
            event->accept();
        } else QGraphicsView::wheelEvent(event);  // Default behavior
    }
private:
    const double zoomFactor;
    double currentZoom = 1.0;
    double minZoom = 0.6;
    double maxZoom = 2.3;
};

#endif // STRUCTURES_H
