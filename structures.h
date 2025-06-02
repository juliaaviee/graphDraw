#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QApplication>
#include <QGraphicsEllipseItem>
#include <QEvent>
#include <QGraphicsTextItem>
#include <QFont>
#include <QFontMetricsF>
#include <QRectF>
#include <QPen>
#include <QPainter>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QLineEdit>
#include <QPointF>
#include <QPolygonF>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QGestureEvent>
#include <QSpinBox>
#include <QPinchGesture>

class Node;

class WeightLabel : public QGraphicsTextItem {
public:
    WeightLabel(const QString& label, QGraphicsItem* parent) : QGraphicsTextItem(parent), weight(label) {
        setFont(QFont("Arial", 10));
        setPlainText(label);
    }
    QRectF boundingRect() const override {
        // Return the base bounding rect with optional margin
        QRectF base = QGraphicsTextItem::boundingRect();
        return base.adjusted(-2, -2, 2, 2); // Add margin for padding/border
    }
protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override {
        // Calculate text bounds
        QRectF rect = boundingRect();
        // Draw background rectangle
        painter->setBrush(Qt::black);
        painter->setPen(QPen(Qt::black, 1)); // Optional border
        painter->drawRect(rect);
        QGraphicsTextItem::paint(painter, option, widget); // Use base implementation
    }
private:
    QString weight;
};

class Edge : public QObject, public QGraphicsLineItem {
    Q_OBJECT
public:
    Edge(Node* sourceNode, Node* destNode, int w=1);
    Node* getSource() {return source;}
    Node* getDest() {return dest;}
    void updatePos();
    void highlightEdge();
    void clearEdge();
    void setWeight(int w);
    int getWeight() {return weight;}
    void directionToggle(bool t) {directed = t; updatePos();}
    bool isDirected() {return directed;}
    void setCounterpart(Edge* c) {ctrpart = c; updatePos();}
    Edge* getCounterpart() {return ctrpart;}
    WeightLabel* getLabel() {return label;}
signals:
    void selected(Edge* e);
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
private:
    WeightLabel* label;
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
    QString getLabel() {return labelItem->toPlainText();}
    QList<Edge*> getCons() {return cons;}
    void addEdge(Edge* e) {cons.append(e);}
    void removeEdge(Edge* e) {cons.removeAt(cons.lastIndexOf(e));}
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    QRectF boundingRect() const override;
signals:
    void selected(Node* node);
    void edit(Node* node);
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
    GraphView(QWidget *parent = nullptr): QGraphicsView(parent), zoomFactor(1.15) {
        grabGesture(Qt::PinchGesture);
        setAttribute(Qt::WA_AcceptTouchEvents);
        viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
        setInteractive(true); // Just in case
    }
    bool event(QEvent *event) override {
        if (event->type() == QEvent::NativeGesture) {
            QNativeGestureEvent *gestureEvent = static_cast<QNativeGestureEvent *>(event);

            if (gestureEvent->gestureType() == Qt::ZoomNativeGesture) {
                qreal delta = gestureEvent->value(); // Usually between -1 and 1
                const qreal scaleFactor = 1 + delta; // Convert to scaling factor

                // Clamp zoom
                double newZoom = currentZoom * scaleFactor;
                if (newZoom < minZoom || newZoom > maxZoom)
                    return true;

                scale(scaleFactor, scaleFactor);
                currentZoom = newZoom;
                return true;
            }
        }
        return QGraphicsView::event(event);
    }

private:
    const double zoomFactor;
    double currentZoom = 1.0;
    double minZoom = 0.6;
    double maxZoom = 2.3;
};

#endif // STRUCTURES_H
