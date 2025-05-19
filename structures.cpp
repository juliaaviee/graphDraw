#include <structures.h>

// Node constructor
Node::Node(const QString& label, QGraphicsItem* parent)
    : QObject(), QGraphicsEllipseItem(parent), labelItem(new QGraphicsTextItem(this)) {

    QFont font("Arial", 12);
    labelItem->setPlainText(label);
    labelItem->setFont(font);
    QFontMetricsF metrics(font);
    int textWidth = metrics.horizontalAdvance(label) >= 30 ? metrics.horizontalAdvance(label)+20 : 40;
    // Node size and flags setup
    setRect(-20, -20, textWidth, 40);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    switch(textWidth) {
    case 40:
        labelItem->setPos(-labelItem->boundingRect().width()/2, -12);
        break;
    default:
        labelItem->setPos(rect().topLeft().x()/1.6, -12);
    }
}

void Node::setLabel(const QString& newLabel) {
    prepareGeometryChange();
    labelItem->setPlainText(newLabel);
    QFontMetricsF metrics(labelItem->font());
    int textWidth = metrics.horizontalAdvance(newLabel) >= 30 ? metrics.horizontalAdvance(newLabel)+20 : 40;
    // Node size and flags setup
    setRect(-20, -20, textWidth, 40);
    switch(textWidth) {
        case 40:
            labelItem->setPos(-labelItem->boundingRect().width()/2, -12);
            break;
        default:
            labelItem->setPos(rect().topLeft().x()/1.6, -12);
    }
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

void Node::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if(event->button() == Qt::RightButton) {
        emit edit(this);
    }
    QGraphicsEllipseItem::mousePressEvent(event);
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
    qreal extra = isSelected() ? 2 : 1;  //Dynamically adjust boundingRect so that it leaves no traces when selected
    return rect().adjusted(-extra, -extra, extra, extra);
}

void Node::removeEdge(Edge* e) {
    cons.removeAt(cons.lastIndexOf(e));
}
Edge::Edge(Node* sourceNode, Node* destNode, int w) : source(sourceNode), dest(destNode), weight(w){
    setFlag(QGraphicsItem::ItemIsSelectable);
    setPen(QPen(Qt::black, 3));
    label = new WeightLabel(QString::number(weight), this);
    QRectF sourceRect = source->sceneBoundingRect();
    QRectF destRect = dest->sceneBoundingRect();

    QPointF cS = sourceRect.center();
    QPointF cD = destRect.center();
    QLineF line(cS, cD);

    // Offset for parallel edges
    QPointF normal(-line.dy(), line.dx());
    normal /= std::sqrt(QPointF::dotProduct(normal, normal));
    qreal n = ctrpart ? (weight != ctrpart->getWeight() ? 10 : 0) : 0;
    QPointF offset = normal * n;

    // Compute direction
    QPointF direction = line.unitVector().p2() - line.unitVector().p1();
    QPointF normDir = direction / std::sqrt(QPointF::dotProduct(direction, direction));

    // Compute radius along the direction of the edge
    qreal radiusS = std::sqrt(
        std::pow(normDir.x() * sourceRect.width() / 2.0, 2) +
        std::pow(normDir.y() * sourceRect.height() / 2.0, 2)
        );
    qreal radiusD = std::sqrt(
        std::pow(normDir.x() * destRect.width() / 2.0, 2) +
        std::pow(normDir.y() * destRect.height() / 2.0, 2)
        );

    QPointF start = cS + normDir * radiusS + offset;
    QPointF end = cD - normDir * radiusD + offset;
    setLine(QLineF(start, end));

    // Center the label
    QPointF center = (start + end) / 2;
    QRectF labelRect = label->boundingRect();
    label->setPos(center.x() - labelRect.width() / 2, center.y() - labelRect.height() / 2);
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
    QRectF sourceRect = source->sceneBoundingRect();
    QRectF destRect = dest->sceneBoundingRect();

    QPointF cS = sourceRect.center();
    QPointF cD = destRect.center();
    QLineF line(cS, cD);

    // Offset for parallel edges
    QPointF normal(-line.dy(), line.dx());
    normal /= std::sqrt(QPointF::dotProduct(normal, normal));
    qreal n = ctrpart ? (weight != ctrpart->getWeight() ? 10 : 0) : 0;
    QPointF offset = normal * n;

    // Compute direction
    QPointF direction = line.unitVector().p2() - line.unitVector().p1();
    QPointF normDir = direction / std::sqrt(QPointF::dotProduct(direction, direction));

    // Compute radius along the direction of the edge
    qreal radiusS = std::sqrt(
        std::pow(normDir.x() * sourceRect.width() / 2.0, 2) +
        std::pow(normDir.y() * sourceRect.height() / 2.0, 2)
        );
    qreal radiusD = std::sqrt(
        std::pow(normDir.x() * destRect.width() / 2.0, 2) +
        std::pow(normDir.y() * destRect.height() / 2.0, 2)
        );

    QPointF start = cS + normDir * radiusS + offset;
    QPointF end = cD - normDir * radiusD + offset;
    setLine(QLineF(start, end));

    // Center the label
    QPointF center = (start + end) / 2;
    QRectF labelRect = label->boundingRect();
    label->setPos(center.x() - labelRect.width() / 2, center.y() - labelRect.height() / 2);
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
WeightLabel* Edge::getLabel() {
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

