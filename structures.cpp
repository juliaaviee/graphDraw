#include <structures.h>


// Node constructor
Node::Node(const QString& label, QGraphicsItem* parent)
    : QObject(), QGraphicsEllipseItem(parent), labelItem(new QGraphicsTextItem(this)) {

    // Node size and flags setup
    setRect(-20, -20, 40, 40);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);


    // Node label setup
    labelItem->setPlainText(label);
    labelItem->setFont(QFont("Arial", 12));

    // Center label inside the node
    QRectF textRect = labelItem->boundingRect();
    labelItem->setPos(-textRect.width() / 2, -textRect.height() / 2);
}


void Node::setLabel(const QString& newLabel) {
    labelItem->setPlainText(newLabel);

    // Adjust label position
    QRectF textRect = labelItem->boundingRect();
    labelItem->setPos(-textRect.width() / 2, -textRect.height() / 2);
}

QString Node::getLabel() {
    return labelItem->toPlainText();
}

void Node::addEdge(Edge* e) {
    cons.append(e);
}

QList<Edge*> Node::getCons() {
    return cons;
}

//itemChange was overriden so that the nodes handle related edges dynamically, and emit a signal whenever selected for making connections through MainWindow's nodeInteraction slot
QVariant Node::itemChange(GraphicsItemChange change, const QVariant &value) {
    if(change == QGraphicsItem::ItemPositionChange) {
        for(Edge* e: cons) {
            e->updatePos();
        }
    }
    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        if (isSelected()) {
            emit selected(this); // Emit signal when selected
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

        QColor borderColor = Qt::black;
        int borderWidth = 2;
        // Check if selected
        if (isSelected()) {
            borderColor = Qt::red;   // Change border color
            borderWidth = 4;            // Make border thicker
        }

        painter->setPen(QPen(borderColor, borderWidth));
        painter->drawEllipse(rect());
}

void Node::removeEdge(Edge* e) {
    cons.removeAt(cons.lastIndexOf(e));
}
Edge::Edge(Node* sourceNode, Node* destNode) : source(sourceNode), dest(destNode) {
    setLine(QLineF(sourceNode->pos(), destNode->pos()));
}

Node* Edge::getSource() {
    return this->source;
}
Node* Edge::getDest() {
    return this->dest;
}

void Edge::updatePos() {
    setLine(QLineF(source->pos(), dest->pos()));
}


