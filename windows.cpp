#include "windows.h"
#include "ui_mainwindow.h"
unsigned short defaultNodeLabel{1}, i{};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , main_ui(new Ui::MainWindow)
{
    main_ui->setupUi(this);
    setWindowTitle("graphDraw");
    setWindowIcon(QIcon(":/graph.png"));
    setFixedSize(800,590);

    scene = new QGraphicsScene(this);
    main_ui->graphicsView->setScene(scene);

    name = main_ui->nodeName;
    newname = main_ui->newName;
    src = main_ui->src;
    dst = main_ui->dst;

    cost = main_ui->sortCost;
    length = main_ui->sortLen;

    changeName = main_ui->changeName;
    showRoutes = main_ui->routes;
    showMatrix = main_ui->showMatrix;
    complete = main_ui->complete;

    weightV = main_ui->weightV;
    weightP = main_ui->weightP;
    weightV->setChecked(true);
    weightP->setChecked(true);
    main_ui->enWeight->setChecked(true);
    main_ui->enDirection->setChecked(true);

    main_ui->insertionEr->setVisible(false);
    main_ui->insertionEr1->setVisible(false);
    main_ui->matrixEr->setVisible(false);
    nodeAmount = main_ui->insertAmount;

    QButtonGroup* sorting = new QButtonGroup(this);
    sorting->addButton(cost);
    sorting->addButton(length);
    length->setChecked(true);

    connectionS = new QShortcut(QKeySequence("Ctrl+Z"), this);
    MainWindow::connect(connectionS, &QShortcut::activated, this, [this]() {
        canDelete = false;
        main_ui->delMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: red;}");
        if(canConnect==false) {
            canConnect = true;
            main_ui->conMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: green;}");
        }
        else {
            canConnect = false;
            main_ui->conMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: red;}");
        }
    });

    deletionS = new QShortcut(QKeySequence("Ctrl+X"), this);
    MainWindow::connect(deletionS, &QShortcut::activated, this, [this]() {
        canConnect = false;
        main_ui->conMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: red;}");
        if(canDelete==false) {
            canDelete = true;
            main_ui->delMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: green;}");
        }
        else {
            canDelete = false;
            main_ui->delMode->setStyleSheet("QCheckBox {color: black;} QCheckBox::indicator {background-color: red;}");
        }
    });

    MainWindow::connect(changeName, &QPushButton::clicked, this, [this]() {
        if(cur) {
            QString l = newname->toPlainText();
            if(l.isEmpty()) return;
            for(Node* no: nodes) {
                if(no->getLabel()==l) {
                    main_ui->insertionEr1->setVisible(true);
                    QTimer::singleShot(2000,main_ui->insertionEr1, &QLabel::hide);
                    return;
                }
            }
            cur->setLabel(l);
        }
    });
    MainWindow::connect(showRoutes, &QPushButton::clicked, this, [this]() {
        QString s = src->toPlainText();
        QString d = dst->toPlainText();
        if(s.isEmpty()||d.isEmpty()) {
            main_ui->routeEr->setVisible(true);
            main_ui->routeEr->setText("*Nó(s) faltando");
            QTimer::singleShot(2000, main_ui->routeEr, &QLabel::hide);
            return;
        }
        if(s==d) {
            main_ui->routeEr->setVisible(true);
            main_ui->routeEr->setText("*Origem igual ao destino");
            QTimer::singleShot(2000, main_ui->routeEr, &QLabel::hide);
            return;
        }
        Node* st=nullptr;
        Node* f=nullptr;
        std::unordered_map<Node*,bool> vis;
        std::unordered_map<QString, Node*> b;
        for(Node* no: nodes) {
            if(no->getLabel()==s) st = no;
            else if(no->getLabel()==d) f = no;
            vis[no] = false;
            b[no->getLabel()] = no;
        }
        if(!st||!f) {
            main_ui->routeEr->setVisible(true);
            main_ui->routeEr->setText("*Rótulo(s) inválido(s)");
            QTimer::singleShot(2000, main_ui->routeEr, &QLabel::hide);
            return;
        }
        QList<Route> routes;
        Route r;

        if(main_ui->enWeight->checkState()) traversal(st,f, r, 0, routes,vis,b);
        else traversal(st,f,r,routes,vis,b);
        if(routes.empty()) {
            qDebug() << "No possible routes from" << s << "to" << d;
            return;
        }
        if(length->isChecked()) {
            std::sort(routes.begin(), routes.end(), [](const Route &a, const Route &b) {
                return a.visual.size() < b.visual.size();  // sort by length
            });
        } else {
            std::sort(routes.begin(), routes.end(), [](const Route &a, const Route &b) {
                return a.totalCost < b.totalCost;  // sort by cost
            });
        }
        if(routeWindow) {routeWindow->close(); delete routeWindow;}
        routeWindow = new RouteWindow(routes, main_ui->enWeight->checkState());
        routeWindow->show();
        routeWindow->raise();
    });
    MainWindow::connect(complete, &QPushButton::clicked, this, [this]() {
        int size = nodes.size();
        if(main_ui->enDirection->checkState()){
            for(short i =0;i<size;i++) {
                QList<Edge*> cons = nodes[i]->getCons();
                for(short j = 0;j<size;j++) {
                    if(nodes[i]==nodes[j]) continue;
                    bool valid=true;
                    for(Edge* e: cons) if(e->getSource()==nodes[i]&&e->getDest()==nodes[j]) {valid =false; break;}
                    if(valid) {
                        Edge* e = new Edge(nodes[i], nodes[j]);
                        MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                        e->directionToggle(main_ui->enDirection->checkState());
                        nodes[i]->addEdge(e);
                        nodes[j]->addEdge(e);
                        scene->addItem(e);
                        if(matrixWindow) matrixWindow->addCon(e);
                        if(!weightV->checkState()) e->getLabel()->setVisible(false);
                    }
                }
            }
        } else {
            for(short i =0;i<size;i++) {
                QList<Edge*> cons = nodes[i]->getCons();
                for(short j = 0;j<size;j++) {
                    if(nodes[i]==nodes[j]) continue;
                    bool valid=true;
                    for(Edge* e: cons) if((e->getSource()==nodes[i]&&e->getDest()==nodes[j]) || (e->getDest()==nodes[i]&&e->getSource()==nodes[j])) {valid =false; break;}
                    if(valid) {
                        Edge* e = new Edge(nodes[i], nodes[j]);
                        MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                        e->directionToggle(main_ui->enDirection->checkState());
                        nodes[i]->addEdge(e);
                        nodes[j]->addEdge(e);
                        scene->addItem(e);
                        if(matrixWindow) matrixWindow->addCon(e);
                        if(!weightV->checkState()) e->getLabel()->setVisible(false);
                    }
                }
            }
        }
    });
}

MainWindow::~MainWindow()
{
    delete main_ui;
}

void MainWindow::traversal(Node* c, Node* d, Route r, int sum, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b) {
    if(v[c]) return; // If it has been visited already, end recursion
    if(c==d) {
        // If we have arrived at our destination, concatenate its label to the end of the string and append the route to routes, then end recursion
        r.visual += c->getLabel();
        r.totalCost = sum;
        r.edges = backtrack(r.visual,b, main_ui->enDirection->checkState());
        rs.append(r);
        return;
    }
    v[c] = true; // Set the current node as visited
    r.visual += c->getLabel() + QString::fromStdString("->");
    QList<Edge*> es = c->getCons();
    if(main_ui->enDirection->checkState()) {
        for(Edge* e: es) {
            Node* ds = e->getDest();
            if(c!=ds) traversal(ds,d,r,sum+e->getWeight(),rs,v,b); // If the destination of the current edge isn't the current node, visit it
        }
    } else {
        for(Edge* e: es) {
            Node* ds = e->getDest();
            if(c!=ds) traversal(ds,d,r,sum+e->getWeight(),rs,v,b); // If the destination of the current edge isn't the current node, visit it
            else traversal(e->getSource(),d,r,sum+e->getWeight(),rs,v,b);
        }
    }
}

//Overload for non-weighted graphs
void MainWindow::traversal(Node* c, Node* d, Route r, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b) {
    if(v[c]) return; // If it has been visited already, end recursion
    if(c==d) {
        // If we have arrived at our destination, concatenate its label to the end of the string and append the route to routes, then end recursion
        r.visual += c->getLabel();
        r.edges = backtrack(r.visual,b, main_ui->enDirection->checkState());
        rs.append(r);
        return;
    }
    v[c] = true; // Set the current node as visited
    r.visual += c->getLabel() + QString::fromStdString("->");
    QList<Edge*> es = c->getCons();
    if(main_ui->enDirection->checkState()) {
        for(Edge* e: es) {
            Node* ds = e->getDest();
            if(c!=ds) traversal(ds,d,r,rs,v,b); // If the destination of the current edge isn't the current node, visit it
        }
    } else {
        for(Edge* e: es) {
            Node* ds = e->getDest();
            if(c!=ds) traversal(ds,d,r,rs,v,b); // If the destination of the current edge isn't the current node, visit it
            else if(c!=e->getSource()) traversal(e->getSource(),d,r,rs,v,b);
        }
    }
}
//The backtrack function was created to ensure we get the correct edges that are to be highlighted, since we had a few issues with traversal.
//It is called when we find our destination in the traversal recursion
QList<Edge*> MainWindow::backtrack(const QString &r, std::unordered_map<QString, Node *> &b, bool dir) {
    QList<Edge*> ans;
    QStringList t = r.split("->");
    int s = t.size()-1;
    if(dir) {
        for(int l = 0;l<s;l++) {
            QList<Edge*> re = b[t[l]]->getCons();
            for(auto e: re) {
                if(b[t[l+1]] == e->getDest()) {
                    ans.append(e);
                    break;
                }
            }
        }
    } else {
        for(int l = 0;l<s;l++) {
            QList<Edge*> re = b[t[l]]->getCons();
            for(auto e: re) {
                if((b[t[l]] == e->getSource() && b[t[l+1]] == e->getDest()) || (b[t[l]] == e->getDest() && b[t[l+1]] == e->getSource())) {
                    ans.append(e);
                    break;
                }
            }
        }
    }
    return ans;
}
void MainWindow::on_insertNode_clicked()
{
    QString l = name->toPlainText();
    if(nodeAmount->currentIndex()>0) { //If the user wants to insert more than 1 node in one go
        if(!l.isEmpty()) {
            for(Node* no: nodes) {
                if(l==no->getLabel()) {
                    main_ui->insertionEr->setVisible(true);
                    QTimer::singleShot(2000, main_ui->insertionEr, &QLabel::hide);
                    return;
                }
            }
            Node *node = new Node(l);
            scene->addItem(node);
            MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
            nodes.append(node);
            i++;
            return;
        }
        int inc{};
        qreal x{}, y{};
        while(inc < nodeAmount->currentIndex()+1) {
            //This loop does check for duplicates yet, something to keep an eye on
            QString num = QString::number(defaultNodeLabel);
            Node *node = new Node(num);
            node->setPos(x,y);
            scene->addItem(node);
            MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
            nodes.append(node);
            defaultNodeLabel++;
            i++;
            inc++;
            x+=50;
            y+=50;
        }
        main_ui->insertionEr->setVisible(false);
        return;
    }
    if(l.isEmpty()) {
        QString num = QString::number(defaultNodeLabel);
        for(Node* no: nodes) {
            if(num==no->getLabel()) {
                main_ui->insertionEr->setVisible(true);
                QTimer::singleShot(2000, main_ui->insertionEr, &QLabel::hide);
                return;
            }
        }
        Node *node = new Node(num);
        scene->addItem(node);
        MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
        nodes.append(node);
        defaultNodeLabel++;
        i++;
    } else {
        for(Node* no: nodes) {
            if(l==no->getLabel()) {
                main_ui->insertionEr->setVisible(true);
                QTimer::singleShot(2000, main_ui->insertionEr, &QLabel::hide);
                return;
            }
        }
        Node *node = new Node(l);
        scene->addItem(node);
        MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
        nodes.append(node);
        i++;
    }
    if(matrixWindow) matrixWindow->close();
}

void MainWindow::nodeInteraction(Node *node) {
    cur = node;
    if(canConnect) {
        if(!tmp) tmp = node;
        else if(tmp != node) {
            QList<Edge*> edges = tmp->getCons();
            if(!main_ui->enDirection->checkState()) {
                for(Edge* e: edges) {
                    //This loop ensures this isn't a connection we've made before
                    if((tmp == e->getSource() && node == e->getDest()) || (tmp == e->getDest() && node == e->getSource())) {
                        tmp = nullptr;
                        return;
                    }
                }
            } else {
                for(Edge* e: edges) {
                    //This loop ensures this isn't a connection we've made before
                    if(tmp == e->getSource() && node == e->getDest()) {
                        tmp = nullptr;
                        return;
                    }
                }
            }
            Edge* e = new Edge(tmp, node);
            MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
            e->directionToggle(main_ui->enDirection->checkState());
            tmp->addEdge(e);
            node->addEdge(e);
            scene->addItem(e);
            if(matrixWindow) matrixWindow->addCon(e);
            tmp = nullptr;
            if(!weightV->checkState()) e->getLabel()->setVisible(false);
            if(weightP->checkState()) {
                if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
                edgeWindow = new EdgeWindow(e);
                edgeWindow->show();
                edgeWindow->raise();
            }
        }
    } else tmp = nullptr;
    if(canDelete) {
        QList<Edge*> edges = node->getCons();
        for(Edge* e: edges) {
            Node* s = e->getSource();
            Node* d = e->getDest();
            if(node!=s) {
                QList<Edge*> es = s->getCons();
                for(Edge* c: es) {
                    if(c==e) {
                        s->removeEdge(c);
                        break;
                    }
                }
            }
            if(node!=d) {
                QList<Edge*> es = d->getCons();
                for(Edge* c: es) {
                    if(c==e) {
                        d->removeEdge(c);
                        break;
                    }
                }
            }
            scene->removeItem(e);
            delete e;
        }
        scene->removeItem(node);
        nodes.removeAt(nodes.lastIndexOf(node));
        delete node;
        i--;
        if(matrixWindow) matrixWindow->close();
    }
}

void MainWindow::edgeInteraction(Edge *e) {
    if(canDelete) {
        if(matrixWindow) {matrixWindow->removeCon(e); return;}
        e->getSource()->removeEdge(e);
        e->getDest()->removeEdge(e);
        scene->removeItem(e);
        delete e;
        if(routeWindow) routeWindow->close();
        return;
    }
    if(main_ui->enWeight->checkState()) {
        if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
        edgeWindow = new EdgeWindow(e);
        edgeWindow->show();
        edgeWindow->raise();
    }
}

void MainWindow::on_showMatrix_clicked()
{
    if(nodes.size()<2) {
        main_ui->matrixEr->setVisible(true);
        QTimer::singleShot(2000, main_ui->matrixEr, &QLabel::hide);
        return;
    }
    std::unordered_map<QString, int> idx; //Unordered map is useful for mapping labels to indexes on a string
    std::unordered_map<int, Node*> n_map; //Mapping index to node for connection handling in matrix window
    unsigned short cnt{};
    for(Node* no: nodes) {
        //Mapping labels to index
        idx[no->getLabel()] = cnt;
        n_map[cnt] = no;
        cnt++;
    }
    //Refactoring matrix building
    QList<QPair<QString, QString>> matrix(cnt, {"", QString(i,'0')});
    int inc{};
    //World's most illegible while loop
    while(inc<cnt) {
        matrix[inc].first = nodes[inc]->getLabel();
        matrix[inc].second[idx[matrix[inc].first]] = 'X';
        QList<Edge*> es = nodes[inc]->getCons();
        if(main_ui->enDirection->checkState()) {for(Edge* e: es) if(e->getDest()!=nodes[inc]) matrix[inc].second[idx[e->getDest()->getLabel()]] = '1';}
        else {
            for(Edge* e: es) {
                matrix[idx[e->getSource()->getLabel()]].second[idx[e->getDest()->getLabel()]] = '1';
                matrix[idx[e->getDest()->getLabel()]].second[idx[e->getSource()->getLabel()]] = '1';
            }
        }
        inc++;
    }
    if(matrixWindow) {matrixWindow->close(); delete matrixWindow;}
    matrixWindow = new MatrixWindow(cnt,cnt,matrix,n_map, main_ui->enDirection->checkState());
    matrixWindow->show();
    matrixWindow->raise();
    connect(matrixWindow->getModel(), &QStandardItemModel::dataChanged, this, &MainWindow::matrixResponse);
}
void MainWindow::matrixResponse(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
    Q_UNUSED(bottomRight);
    QModelIndex idx = matrixWindow->getModel()->index(topLeft.row(), topLeft.column());
    QString comp = matrixWindow->getModel()->data(idx, Qt::DisplayRole).toString();
    auto &nref = matrixWindow->getNodeMap();
    auto &eref = matrixWindow->getCellCons();
    std::pair<int,int> key(topLeft.row(),topLeft.column());
    if(comp == "1")
    {
        if(eref[key]!=nullptr) return; //If the connection has already been made, ignore
        //Otherwise, execute proper operations
        Edge* e = new Edge(nref[topLeft.row()], nref[topLeft.column()]);
        nref[topLeft.row()]->addEdge(e);
        nref[topLeft.column()]->addEdge(e);
        eref[key] = e;
        scene->addItem(e);
        connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
        if(!weightV->checkState()) e->getLabel()->setVisible(false);
        if(weightP->checkState()) {
            if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
            edgeWindow = new EdgeWindow(e);
            edgeWindow->show();
            edgeWindow->raise();
        }
        if(!main_ui->enDirection->checkState()) {
            e->directionToggle(false);
            eref[std::pair(topLeft.column(),topLeft.row())] = e;
            matrixWindow->getModel()->setData(matrixWindow->getModel()->index(topLeft.column(), topLeft.row()), "1", Qt::EditRole);
        }
    }
    else
    {
        if(!eref[key]) return; //If there was no connection to begin with, ignore
        nref[topLeft.row()]->removeEdge(eref[key]);
        nref[topLeft.column()]->removeEdge(eref[key]);
        scene->removeItem(eref[key]);
        eref[key] = nullptr;
        if(!main_ui->enDirection->checkState()) {
            eref[std::pair(topLeft.column(), topLeft.row())] = nullptr;
            matrixWindow->getModel()->setData(matrixWindow->getModel()->index(topLeft.column(), topLeft.row()), "0", Qt::EditRole);
        }
    }
}
void MainWindow::on_reset_clicked()
{
    if(routeWindow) {routeWindow->close(); delete routeWindow;}
    if(matrixWindow) {matrixWindow->close();}
    if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
    for(auto it: scene->items()) delete it;
    scene->clear();
    nodes.clear();
    defaultNodeLabel = 1;
    i=0;
}

void MainWindow::on_weightV_stateChanged(int b)
{
    for(auto it: scene->items()) {
        Edge* e = qgraphicsitem_cast<Edge*>(it);
        if(e) e->getLabel()->setVisible(b);
    }
}
void MainWindow::on_enWeight_stateChanged(int b)
{
    if(b) {
        weightP->setChecked(true);
        weightV->setChecked(true);
        weightP->setVisible(true);
        weightV->setVisible(true);
    } else {
        weightP->setChecked(false);
        weightV->setChecked(false);
        weightP->setVisible(false);
        weightV->setVisible(false);
    }
    if(routeWindow) routeWindow->close();
    cost->setVisible(b);
    if(!b) length->setChecked(true);
}

void MainWindow::on_enDirection_stateChanged(int b)
{
    if(routeWindow) routeWindow->close();
    if(matrixWindow) matrixWindow->toggleDirection(main_ui->enDirection->checkState());
    int s = scene->items().size();
    QList<QGraphicsItem*> l = scene->items();
    if(b) {
        for(int i = 0;i<s;i++) {
            Edge* e = qgraphicsitem_cast<Edge*>(l[i]);
            if(e) {
                e->directionToggle(true);
                Edge* E = new Edge(e->getDest(), e->getSource());
                e->getDest()->addEdge(E);
                e->getSource()->addEdge(E);
                MainWindow::connect(E, &Edge::selected, this, &MainWindow::edgeInteraction);
                E->getLabel()->setVisible(weightV->checkState());
                if(matrixWindow) matrixWindow->addCon(E);
                scene->addItem(E);
            }
        }
    } else {
        for(int i = 0;i<s;i++) {
            Edge* e = qgraphicsitem_cast<Edge*>(l[i]);
            if(e) {
                for(int j = i+1;j<s;j++) {
                    Edge* E = qgraphicsitem_cast<Edge*>(l[j]);
                    if(E) {
                        //If it's the counterpart to the l[i] edge, remove it
                        if(e->getSource()==E->getDest()&&e->getDest()==E->getSource()) {
                            E->getSource()->removeEdge(E);
                            E->getDest()->removeEdge(E);
                            scene->removeItem(E);
                            delete E;
                        }
                    }
                }
                e->directionToggle(false);
                if(matrixWindow) matrixWindow->addCon(e);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //Ensuring children windows are closed
    if(routeWindow) routeWindow->close();
    if(matrixWindow) matrixWindow->close();
    if(edgeWindow) edgeWindow->close();
    event->accept();
}
MatrixWindow::MatrixWindow(unsigned short r, unsigned short c, const QList<QPair<QString, QString>> &m, const std::unordered_map<int, Node*> &nMap, bool dir, QWidget *parent) : QDialog(parent), nodeMap(nMap), directed(dir){
    setWindowTitle("Matriz adj.");
    setFixedSize(500,300);
    setWindowIcon(QIcon(":/matrix.png"));
    view = new QTableView(this);
    model = new QStandardItemModel(r,c,this);
    short k{};
    short s = m.size();
    while(k<s) {
        //Setting headers
        model->setHeaderData(k, Qt::Horizontal, m[k].first);
        model->setHeaderData(k, Qt::Vertical, m[k].first);
        for(short co = 0;co<r;co++) {
            //Looping through the row and populating table
            QStandardItem *cur = new QStandardItem(m[k].second[co]);
            if(co==k) cur->setFlags(cur->flags()&~Qt::ItemIsEditable&~Qt::ItemIsSelectable); //If it's part of the diagonal, make it not editable nor selectable
            //If x connects to y, search through x's connections to find the related edge
            if(cur->text() == "1") {
                QList<Edge*> es = nodeMap[k]->getCons();
                if(directed) {
                    for(auto e: es) {
                        if(e->getDest() == nodeMap[co]) {
                            cellCons[std::pair(k,co)] = e;
                            break;
                        }
                    }
                } else {
                    for(auto e: es) {
                        if(e->getDest() == nodeMap[co]) {
                            cellCons[std::pair(k,co)] = e;
                            cellCons[std::pair(co,k)] = e;
                            break;
                        }
                    }
                }
            }
            cur->setTextAlignment(Qt::AlignCenter);
            model->setItem(k, co, cur);
        }
        k++;
    }
    view->setModel(model);
    view->resize(500,300);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ComboBoxDelegate* delegate = new ComboBoxDelegate({"0", "1"});
    view->setItemDelegate(delegate);
    view->setEditTriggers(QAbstractItemView::AllEditTriggers); //Items access edit mode in one click
}

QStandardItemModel* MatrixWindow::getModel() {
    return model;
}
std::unordered_map<int, Node*>& MatrixWindow::getNodeMap() {
    return nodeMap;
}

void MatrixWindow::toggleDirection(bool dir) {
    directed = dir;
}

void MatrixWindow::removeCon(Edge *e) {
    for(auto c: cellCons) {
        if(c.second == e) {
            c.second = nullptr;
            model->setData(model->index(c.first.first, c.first.second), "0", Qt::EditRole);
            if(!directed) {
                cellCons[std::pair(c.first.second, c.first.first)] = nullptr;
                model->setData(model->index(c.first.second, c.first.first), "0", Qt::EditRole);
            }
        }
    }
}

void MatrixWindow::addCon(Edge* e) {
    int r{-1}, c{-1};
    Node* s = e->getSource();
    Node* d = e->getDest();
    for(auto n: nodeMap) {
        if(n.second == s) r = n.first;
        else if(n.second == d) c = n.first;
        if(r != -1 && c != -1) break;
    }
    cellCons[std::pair(r,c)] = e;
    model->setData(model->index(r,c), "1", Qt::EditRole);
    if(!directed) {
        cellCons[std::pair(c,r)] = e;
        model->setData(model->index(c,r), "1", Qt::EditRole);
    }
}
std::unordered_map<std::pair<int,int>, Edge*, PairHash>& MatrixWindow::getCellCons() {
    return cellCons;
}
//The ComboBoxDelegate class was made to change the edit mode for each cell in the table view in MatrixWindow
ComboBoxDelegate::ComboBoxDelegate(const QStringList &options, QObject *parent) : QStyledItemDelegate(parent), m_options(options){}

QWidget* ComboBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
    QComboBox *editor = new QComboBox(parent);
    editor->addItems(m_options);
    return editor;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    QComboBox *combo = static_cast<QComboBox*>(editor);
    combo->setCurrentText(index.model()->data(index, Qt::EditRole).toString());
}

void ComboBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
    QComboBox *combo = static_cast<QComboBox*>(editor);
    model->setData(index, combo->currentText(), Qt::EditRole);
}

RouteWindow::RouteWindow(const QList<Route> &rs, bool w, QWidget *parent) : QDialog(parent) , weighted(w){
    setWindowTitle("Rotas: " + QString::number(rs.length()));
    setFixedSize(500,300);
    list = new QListWidget(this);
    if(weighted) {
        for(Route r: rs) {
            //Mapping routes to their respective edges while populating list
            routes[r.visual] = r.edges;
            list->addItem(r.visual + " Cost: " + QString::number(r.totalCost));
        }
    } else {
        for(Route r: rs) {
            //Mapping routes to their respective edges while populating list
            routes[r.visual] = r.edges;
            list->addItem(r.visual);
        }
    }
    list->setFixedSize(500,300);
    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        for(Edge* e: tmp) e->clearEdge();
        QList<Edge*> es = routes[item->text().split(" ")[0]];
        tmp = es;
        for(Edge* e: es) e->highlightEdge();
    });
}

void RouteWindow::closeEvent(QCloseEvent *event) {
    //Overriding the close event for the route window so that there are no leftover highlighted routes
    for(auto r: tmp) r->clearEdge();
    event->accept();
}

EdgeWindow::EdgeWindow(Edge* e, QWidget *parent) : QDialog(parent) {
    setFixedSize(140,25);
    setWindowTitle("Insira o peso");
    w = new QLineEdit(this);
    w->setValidator(new QIntValidator(1,9999, this));
    edge = e;
    EdgeWindow::connect(w, &QLineEdit::returnPressed, this, [this]() {
        QString l = w->text();
        if(l.isEmpty()) close();
        else {
            edge->setWeight(l.toInt());
            close();
        }
    });
}


