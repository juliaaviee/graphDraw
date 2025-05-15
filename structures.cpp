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
    update();
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
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QColor borderColor = Qt::black;
    int borderWidth = 2;
    // Check if selected
    if (isSelected()) {
        borderColor = Qt::red;   // Change border color
        borderWidth = 4;         // Make border thicker
    }
    painter->setPen(QPen(borderColor, borderWidth));
    painter->drawEllipse(rect());
}

QRectF Node::boundingRect() const {
    qreal extra = (isSelected() ? 4 : 2)/2;  //Dynamically adjust boundingRect so that it leaves no traces when selected
    return rect().adjusted(-extra, -extra, extra, extra);
}

void Node::removeEdge(Edge* e) {
    cons.removeAt(cons.lastIndexOf(e));
}
Edge::Edge(Node* sourceNode, Node* destNode, int w) : source(sourceNode), dest(destNode), weight(w){
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 3));
    label = new QGraphicsTextItem(QString::number(weight), this);
    label->setFont(QFont("Arial", 10));
    QPointF cS = source->sceneBoundingRect().center();
    QPointF cD = dest->sceneBoundingRect().center();
    QLineF line(cS, cD);
    QPointF normal = QPointF(-line.dy(), line.dx());
    normal /= std::sqrt(QPointF::dotProduct(normal, normal));
    qreal n = ctrpart ? (weight != ctrpart->getWeight() ? 10 : 0) : 0; //If there's a counterpart, nudge it, otherwise don't
    QPointF offset = normal * n;
    QPointF direction = line.unitVector().p2() - line.unitVector().p1();
    qreal radius = source->boundingRect().width() / 2.0;

    QPointF start = cS + direction * radius+offset;
    QPointF end = cD - direction * radius+offset;
    setLine(QLineF(start,end));
    QPointF center = (start + end) / 2;
    label->setPos(center + QPointF(0, -10));
}

QVariant Edge::itemChange(GraphicsItemChange change, const QVariant &value) {
    if(change == QGraphicsItem::ItemSelectedHasChanged) {
        if(isSelected()) {
            emit selected(this);
        }
    }
    return QGraphicsLineItem::itemChange(change, value);
}
Node* Edge::getSource() {
    return source;
}
Node* Edge::getDest() {
    return dest;
}

Edge* Edge::getCounterpart() {
    return ctrpart;
}
void Edge::updatePos() {
    QPointF cS = source->sceneBoundingRect().center();
    QPointF cD = dest->sceneBoundingRect().center();
    QLineF line(cS, cD);
    QPointF normal = QPointF(-line.dy(), line.dx());
    normal /= std::sqrt(QPointF::dotProduct(normal, normal));
    qreal n = ctrpart ? (weight != ctrpart->getWeight() ? 10 : 0) : 0; //If there's a counterpart, nudge it, otherwise don't
    QPointF offset = normal * n;
    QPointF direction = line.unitVector().p2() - line.unitVector().p1();
    qreal radius = source->boundingRect().width() / 2.0;

    QPointF start = cS + direction * radius+offset;
    QPointF end = cD - direction * radius+offset;
    setLine(QLineF(start,end));
    QPointF center = (start + end) / 2;
    label->setPos(center + QPointF(0, -10));
    update();
}

void Edge::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    // Draw the main line
    painter->setPen(pen());
    painter->drawLine(line());
    if(directed) {
        // Draw the arrowhead
        QLineF l = line();

        double angle = std::atan2(-l.dy(), l.dx()); // angle in radians

        // Arrow size
        const double arrowSize = 10;

        // Arrow tip point
        QPointF arrowP1 = l.p2() - QPointF(std::cos(angle + M_PI / 6) * arrowSize,
                                           -std::sin(angle + M_PI / 6) * arrowSize);
        QPointF arrowP2 = l.p2() - QPointF(std::cos(angle - M_PI / 6) * arrowSize,
                                           -std::sin(angle - M_PI / 6) * arrowSize);

        QPolygonF arrowHead;
        arrowHead << l.p2() << arrowP1 << arrowP2;
        painter->setBrush(QBrush(Qt::black));
        painter->drawPolygon(arrowHead);
    }
}

QRectF Edge::boundingRect() const {
    qreal extra = 10; // match the arrow size
    return QRectF(line().p1(), QSizeF(line().p2().x() - line().p1().x(),
                                      line().p2().y() - line().p1().y()))
        .normalized()
        .adjusted(-extra, -extra, extra, extra);
}

QPainterPath Edge::shape() const {
    QPainterPath path;
    if(directed){
        // 1. Get the edge line
        QLineF edgeLine = this->line();

        // 2. Calculate the angle of the line
        qreal angle = std::atan2(edgeLine.dy(), edgeLine.dx()); // in radians

        // 3. Define a small rectangle centered at the middle of the line
        QPointF center = edgeLine.pointAt(1); // midpoint

        QRectF rect(-10, -10, 15, 15); // center-based

        // 4. Create a transform to rotate and move the rectangle
        QTransform transform;
        transform.translate(center.x(), center.y());
        transform.rotateRadians(angle);

        // 5. Add the transformed rectangle to the path
        path.addRect(transform.mapRect(rect));
        return path;
    } else {
        path.moveTo(line().p1());
        path.lineTo(line().p2());
        QPainterPathStroker stroker;
        stroker.setWidth(15);  // This defines the selection area
        return stroker.createStroke(path);
    }
    return path;
}
void Edge::setWeight(int w) {
    weight = w;
    if(ctrpart) {
        updatePos();
        ctrpart->updatePos();
    }
    label->setPlainText(QString::number(w));
}

int Edge::getWeight() {
    return weight;
}

void Edge::directionToggle(bool t) {
    directed = t;
    updatePos();
}

void Edge::setCounterpart(Edge* c) {
    ctrpart = c;
    updatePos();
}
QGraphicsTextItem* Edge::getLabel() {
    return label;
}
void Edge::highlightEdge() {
    setPen(QPen(Qt::red, 5));
    setZValue(1);
}

void Edge::clearEdge() {
    setPen(QPen(Qt::black, 3));
    setZValue(0);
}

