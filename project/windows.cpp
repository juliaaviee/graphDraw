#include "windows.h"
#include "ui_mainwindow.h"
#include <random>

unsigned short defaultNodeLabel{1}, nodeCount{};
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> wRange(1,20);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , main_ui(new Ui::MainWindow)
{
    main_ui->setupUi(this);
    setWindowTitle("graphDraw");
    setWindowIcon(QIcon(":/graph.png"));
    setFixedSize(800,590);

    view = new GraphView(this);
    view->resize(802,451);
    scene = new QGraphicsScene(this);
    view->setScene(scene);

    name = main_ui->nodeName;
    src = main_ui->src;
    dst = main_ui->dst;

    cost = main_ui->sortCost;
    length = main_ui->sortLen;

    showRoutes = main_ui->routes;
    showMatrix = main_ui->showMatrix;
    complete = main_ui->complete;

    nodeAmount = new CSpinBox(this);
    nodeAmount->setRange(1,10);
    nodeAmount->move(125,525);
    nodeAmount->resize(35,25);
    minWRange = new CSpinBox(this);
    maxWRange = new CSpinBox(this);
    minWRange->move(500,530);
    minWRange->setValue(1);
    maxWRange->move(580,530);
    maxWRange->setValue(20);
    minWRange->setRange(1,maxWRange->value());
    minWRange->resize(50,25);
    maxWRange->resize(50,25);
    maxWRange->setRange(minWRange->value(),9999);
    weightV = main_ui->weightV;
    weightP = main_ui->weightP;
    weightV->setChecked(true);
    weightP->setChecked(true);
    main_ui->enWeight->setChecked(true);
    main_ui->enDirection->setChecked(true);

    main_ui->conMode->setParent(this);
    main_ui->delMode->setParent(this);
    main_ui->conMode->raise();
    main_ui->delMode->raise();

    main_ui->insertionEr->setVisible(false);
    main_ui->matrixEr->setVisible(false);

    QButtonGroup* sorting = new QButtonGroup(this);
    sorting->addButton(cost);
    sorting->addButton(length);
    length->setChecked(true);

    connectionS = new QShortcut(QKeySequence("Ctrl+A"), this);
    MainWindow::connect(connectionS, &QShortcut::activated, this, [this]() {
        canDelete = false;
        main_ui->delMode->setStyleSheet("QCheckBox::indicator {background-color: red;}");
        if(canConnect==false) {
            canConnect = true;
            main_ui->conMode->setStyleSheet("QCheckBox::indicator {background-color: green;}");
        }
        else {
            canConnect = false;
            main_ui->conMode->setStyleSheet("QCheckBox::indicator {background-color: red;}");
        }
    });
    revert = new QShortcut(QKeySequence("Ctrl+Z"), this);
    MainWindow::connect(revert, &QShortcut::activated, this, [this](){
        undoStack->undo();
    });
    deletionS = new QShortcut(QKeySequence("Ctrl+X"), this);
    MainWindow::connect(deletionS, &QShortcut::activated, this, [this]() {
        canConnect = false;
        main_ui->conMode->setStyleSheet("QCheckBox::indicator {background-color: red;}");
        if(canDelete==false) {
            canDelete = true;
            main_ui->delMode->setStyleSheet("QCheckBox::indicator {background-color: green;}");
        }
        else {
            canDelete = false;
            main_ui->delMode->setStyleSheet("QCheckBox::indicator {background-color: red;}");
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
            main_ui->routeEr->setVisible(true);
            main_ui->routeEr->setText("Nenhuma rota possível");
            QTimer::singleShot(2000, main_ui->routeEr, &QLabel::hide);
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
                        Edge* e = new Edge(nodes[i], nodes[j], wRange(gen));
                        for(Edge* ct: nodes[i]->getCons()) {
                            if(ct->getSource()==nodes[j]&& ct->getDest()==nodes[i]) {
                                e->setCounterpart(ct);
                                ct->setCounterpart(e);
                            }
                        }
                        MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                        e->directionToggle(true);
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
                        Edge* e = new Edge(nodes[i], nodes[j], wRange(gen));
                        MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                        e->directionToggle(false);
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
    MainWindow::connect(minWRange, &CSpinBox::valueChanged, this, [this](){
        maxWRange->setMinimum(minWRange->value());
        wRange.param(std::uniform_int_distribution<>::param_type(minWRange->value(), wRange.b()));
    });
    MainWindow::connect(maxWRange, &CSpinBox::valueChanged, this, [this](){
        minWRange->setMaximum(maxWRange->value());
        wRange.param(std::uniform_int_distribution<>::param_type(wRange.a(), maxWRange->value()));
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
    QList<QString> ls;
    if(nodeAmount->value()>1) { //If the user wants to insert more than 1 node in one go
        if(!l.isEmpty()) {
            for(Node* no: nodes) {
                if(l==no->getLabel()) {
                    main_ui->insertionEr->setVisible(true);
                    QTimer::singleShot(2000, main_ui->insertionEr, &QLabel::hide);
                    return;
                }
            }
            Node *node = new Node(l);
            ls.append(l);
            scene->addItem(node);
            MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
            MainWindow::connect(node, &Node::edit, this, &MainWindow::nodeLabelEdit);
            nodes.append(node);
            nodeCount++;
            undoStack->push(new AddNodeOP(scene,ls,nodes,nodeCount,this));
            return;
        }
        int inc{};
        qreal x{};
        while(inc < nodeAmount->value()) {
            //This loop does check for duplicates yet, something to keep an eye on
            QString num = QString::number(defaultNodeLabel);
            Node *node = new Node(num);
            ls.append(node->getLabel());
            node->setPos(x,0);
            scene->addItem(node);
            MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
            MainWindow::connect(node, &Node::edit, this, &MainWindow::nodeLabelEdit);
            nodes.append(node);
            defaultNodeLabel++;
            nodeCount++;
            inc++;
            x+=50;
        }
        undoStack->push(new AddNodeOP(scene,ls,nodes,nodeCount,this));
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
        MainWindow::connect(node, &Node::edit, this, &MainWindow::nodeLabelEdit);
        nodes.append(node);
        defaultNodeLabel++;
        nodeCount++;
        ls.append(num);
        undoStack->push(new AddNodeOP(scene,ls,nodes,nodeCount,this));
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
        MainWindow::connect(node, &Node::edit, this, &MainWindow::nodeLabelEdit);
        nodes.append(node);
        nodeCount++;
        ls.append(l);
        undoStack->push(new AddNodeOP(scene,ls,nodes,nodeCount,this));
    }
    if(matrixWindow) matrixWindow->close();
}

void MainWindow::nodeInteraction(Node *node) {
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
            Edge* ctr = nullptr;
            if(main_ui->enDirection->checkState()) {
                //If direction is enabled, check if there exists a counterpart to the edge we are about to make
                for(Edge* e : node->getCons()) {
                    if(e->getSource()==node&&e->getDest()==tmp) {
                        //If it is found, store it and break loop
                        ctr = e;
                        break;
                    }
                }
            }
            Edge* e = new Edge(tmp, node);
            if(ctr) {
                ctr->setCounterpart(e);
                e->setCounterpart(ctr);
            }
            MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
            undoStack->push(new MakeConnectionOP(scene,tmp->getLabel(),node->getLabel(),main_ui->enDirection->checkState(),main_ui->enWeight->checkState(),this));
            e->directionToggle(main_ui->enDirection->checkState());
            tmp->addEdge(e);
            node->addEdge(e);
            scene->addItem(e);
            if(matrixWindow) matrixWindow->addCon(e);
            tmp = nullptr;
            if(!weightV->checkState()) e->getLabel()->setVisible(false);
            if(weightP->checkState()) {
                if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
                edgeWindow = new EdgeWindow(e,scene,undoStack);
                edgeWindow->show();
                edgeWindow->raise();
            }
        }
    } else tmp = nullptr;
    if(canDelete) {
        undoStack->push(new RemoveNodeOP(scene, node,node->pos(),nodes,nodeCount,main_ui->enDirection->checkState(),weightV->checkState(),this));
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
        nodeCount--;
        if(matrixWindow) matrixWindow->close();
        if(nodeWindow) nodeWindow->close();
    }
}

void MainWindow::edgeInteraction(Edge *e) {
    if(canDelete) {
        if(matrixWindow) {
            undoStack->push(new RemoveConnectionOP(scene,e->getSource()->getLabel(),e->getDest()->getLabel(),e->getWeight(),main_ui->enDirection->checkState(),weightV->checkState(),e->getCounterpart()!=nullptr,this));
            matrixWindow->removeCon(e);
            return;
        }
        e->getSource()->removeEdge(e);
        e->getDest()->removeEdge(e);
        undoStack->push(new RemoveConnectionOP(scene,e->getSource()->getLabel(),e->getDest()->getLabel(),e->getWeight(),main_ui->enDirection->checkState(),weightV->checkState(),e->getCounterpart()!=nullptr,this));
        if(e->getCounterpart()) e->getCounterpart()->setCounterpart(nullptr);
        scene->removeItem(e);
        delete e;
        if(routeWindow) routeWindow->close();
        return;
    }
    if(main_ui->enWeight->checkState()) {
        if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
        edgeWindow = new EdgeWindow(e,scene,undoStack);
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
    QList<QPair<QString, QString>> matrix(cnt, {"", QString(nodeCount,'0')});
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

void MainWindow::nodeLabelEdit(Node* n) {
    if(nodeWindow) {nodeWindow->close(); delete nodeWindow;}
    nodeWindow = new NodeWindow(n, nodes, undoStack);
    nodeWindow->show();
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
        undoStack->push(new MakeConnectionOP(scene,nref[topLeft.row()]->getLabel(),nref[topLeft.column()]->getLabel(),main_ui->enDirection->checkState(),main_ui->enWeight->checkState(),this));
        nref[topLeft.row()]->addEdge(e);
        nref[topLeft.column()]->addEdge(e);
        eref[key] = e;
        Edge* ctr = nullptr;
        if(main_ui->enDirection->checkState()) {
            //If direction is enabled, check if there exists a counterpart to the edge we are about to make
            for(Edge* e : nref[topLeft.row()]->getCons()) {
                if(e->getSource()==nref[topLeft.column()]&&e->getDest()==nref[topLeft.row()]) {
                    //If it is found, store it and break loop
                    ctr = e;
                    break;
                }
            }
        }
        if(ctr) {
            e->setCounterpart(ctr);
            ctr->setCounterpart(e);
        }
        scene->addItem(e);
        connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
        if(!weightV->checkState()) e->getLabel()->setVisible(false);
        if(weightP->checkState()) {
            if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
            edgeWindow = new EdgeWindow(e,scene,undoStack);
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
        undoStack->push(new RemoveConnectionOP(scene,eref[key]->getSource()->getLabel(),eref[key]->getDest()->getLabel(),eref[key]->getWeight(),main_ui->enDirection->checkState(),weightV->checkState(),eref[key]->getCounterpart()!=nullptr,this));
        if(eref[key]->getCounterpart()) eref[key]->getCounterpart()->setCounterpart(nullptr); //If there is a counterpart, center it
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
    scene->clear();
    nodes.clear();
    defaultNodeLabel = 1;
    nodeCount=0;
}

void MainWindow::on_save_clicked()
{
    if(scene->items().empty()) {qDebug()<<"Scene is empty"; return;}
    QString content;
    std::unordered_map<int, Node*> node_map;
    std::unordered_map<Node*, int> idx;
    int cnt{};
    for(Node* n: nodes) { //We save all nodes and their respective positions to the first line, while mapping nodes to indexes
        content += n->getLabel() + "," + QString::number(n->pos().x()) + "," + QString::number(n->pos().y()) + "|";
        node_map[cnt] = n;
        idx[n] = cnt;
        cnt++;
    }
    content.removeLast();
    content += "\n" + QString::number(main_ui->enDirection->checkState()) + "\n" + QString::number(main_ui->enWeight->checkState()) + "\n";
    if(cnt>1) {
        QList<QList<QString>> matrix(cnt, QList<QString>(cnt, "0"));
        if(main_ui->enDirection->checkState()) {
            for(auto it: scene->items()) {
                Edge* e = qgraphicsitem_cast<Edge*>(it);
                if(e) matrix[idx[e->getSource()]][idx[e->getDest()]] = QString::number(e->getWeight());
            }
        } else {
            for(auto it: scene->items()) {
                Edge* e = qgraphicsitem_cast<Edge*>(it);
                if(e) {
                    matrix[idx[e->getSource()]][idx[e->getDest()]] = QString::number(e->getWeight());
                    matrix[idx[e->getDest()]][idx[e->getSource()]] = QString::number(e->getWeight());
                }
            }
        }
        for(QList<QString> row: matrix) {
            for(QString col: row) content += col + ",";
            content.removeLast();
            content += "\n";
        }
    }
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save File",
        QDir::homePath(),           // Default path
        "Text Files (*.txt);;All Files (*)"  // File filters
        );

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << content;
            file.close();
        } else qWarning() << "Couldn't save file";
    }

}
void MainWindow::on_load_clicked()
{
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Text File",
        QDir::homePath(),
        "Text Files (*.txt);;All Files (*)"
        );
    if (!filepath.isEmpty()) {
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QList<QString> lines = in.readAll().split("\n");
            QList<QString> coords = lines[0].split("|");
            main_ui->enDirection->setChecked(lines[1].toInt());
            main_ui->enWeight->setChecked(lines[2].toInt());
            file.close();
            on_reset_clicked();
            std::unordered_map<int, Node*> n_map;
            for(QString n: coords) {
                QList<QString> m = n.split(",");
                Node *node = new Node(m[0]);
                scene->addItem(node);
                MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
                MainWindow::connect(node, &Node::edit, this, &MainWindow::nodeLabelEdit);
                nodes.append(node);
                n_map[nodeCount] = node;
                nodeCount++;
                node->setPos(m[1].toDouble(), m[2].toDouble());
            }
            if(nodeCount>1) {
                QList<QList<QString>> matrix;
                for(int c=3;c-3<nodeCount;c++) matrix.append(lines[c].split(",")); //Storing the matrix
                std::unordered_map<std::pair<int,int>, bool, PairHash> cellCons; //Mapping coordinates to a boolean that indicates if we've already made this connection or not
                if(main_ui->enDirection->checkState()){
                    for(int c = 0;c<nodeCount;c++) {
                        for(int j = 0;j<nodeCount;j++) {
                            if(matrix[c][j]!="0" && !cellCons[std::pair(c,j)]) {
                                if(matrix[j][c]!="0" && !cellCons[std::pair(j,c)]) {
                                    Edge* e = new Edge(n_map[c], n_map[j], matrix[c][j].toInt());
                                    Edge* E = new Edge(n_map[j], n_map[c], matrix[j][c].toInt());
                                    e->setCounterpart(E);
                                    E->setCounterpart(e);
                                    e->getLabel()->setVisible(main_ui->enWeight->checkState());
                                    E->getLabel()->setVisible(main_ui->enWeight->checkState());
                                    e->directionToggle(true);
                                    E->directionToggle(true);
                                    n_map[c]->addEdge(e);
                                    n_map[c]->addEdge(E);
                                    n_map[j]->addEdge(e);
                                    n_map[j]->addEdge(E);
                                    MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                                    MainWindow::connect(E, &Edge::selected, this, &MainWindow::edgeInteraction);
                                    scene->addItem(e);
                                    scene->addItem(E);
                                    cellCons[std::pair(c,j)] = true;
                                    cellCons[std::pair(j,c)] = true;
                                } else {
                                    Edge* e = new Edge(n_map[c], n_map[j], matrix[c][j].toInt());
                                    MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                                    e->getLabel()->setVisible(main_ui->enWeight->checkState());
                                    e->directionToggle(true);
                                    n_map[c]->addEdge(e);
                                    n_map[j]->addEdge(e);
                                    scene->addItem(e);
                                    cellCons[std::pair(c,j)]= true;
                                }
                            }
                        }
                    }
                } else {
                    for(int c = 0;c<nodeCount;c++) {
                        for(int j = 0;j<nodeCount;j++) {
                            if(!cellCons[std::pair(c,j)] && matrix[c][j] != "0"){
                                Edge* e = new Edge(n_map[c], n_map[j], matrix[c][j].toInt());
                                MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                                e->getLabel()->setVisible(main_ui->enWeight->checkState());
                                e->directionToggle(false);
                                n_map[c]->addEdge(e);
                                n_map[j]->addEdge(e);
                                scene->addItem(e);
                                cellCons[std::pair(j,c)] = true;
                            }
                        }
                    }
                }
            }
        } else qWarning() << "Could not open file:" << file.errorString();
        return;
    }
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
    weightP->setChecked(b);
    weightV->setChecked(b);
    weightP->setVisible(b);
    weightV->setVisible(b);
    cost->setVisible(b);
    minWRange->setVisible(b);
    maxWRange->setVisible(b);
    main_ui->maxW->setVisible(b);
    main_ui->minW->setVisible(b);
    length->setChecked(!b);
    if(routeWindow) routeWindow->close();
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
                Edge* E = new Edge(e->getDest(), e->getSource(), e->getWeight());
                e->setCounterpart(E);
                E->setCounterpart(e);
                e->getDest()->addEdge(E);
                e->getSource()->addEdge(E);
                MainWindow::connect(E, &Edge::selected, this, &MainWindow::edgeInteraction);
                E->getLabel()->setVisible(weightV->checkState());
                if(matrixWindow) matrixWindow->addCon(E);
                scene->addItem(E);
            }
        }
    } else {
        QList<Edge*> toRemove;
        QList<Edge*> ignore;
        for(auto it: l) { //l contains scene-items()
            Edge* e = qgraphicsitem_cast<Edge*>(it);
            if(e) {
                e->directionToggle(false);
                if(e->getCounterpart() && toRemove.lastIndexOf(e->getCounterpart())==-1&&ignore.lastIndexOf(e->getCounterpart())==-1) { //If the counterpart exists, hasn't been marked for removal already and isn't a protected edge
                    toRemove.append(e->getCounterpart());
                    ignore.append(e);
                    e->setCounterpart(nullptr);
                } else if(!e->getCounterpart()) {
                    if(matrixWindow) matrixWindow->addCon(e);
                }
            }
        }
        for(Edge* e: toRemove) {
            e->getSource()->removeEdge(e);
            e->getDest()->removeEdge(e);
            scene->removeItem(e);
            if(matrixWindow) matrixWindow->addCon(e->getCounterpart());
            delete e;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //Ensuring all windows are closed
    if(routeWindow) routeWindow->close();
    if(matrixWindow) matrixWindow->close();
    if(edgeWindow) edgeWindow->close();
    if(nodeWindow) nodeWindow->close();
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
            model->setData(model->index(c.first.first, c.first.second), "0", Qt::EditRole);
            if(!directed) {
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
            routes[r.visual + " Cost: " + QString::number(r.totalCost)] = r.edges;
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
        QList<Edge*> es = routes[item->text()];
        tmp = es;
        for(Edge* e: es) e->highlightEdge();
    });
}

void RouteWindow::closeEvent(QCloseEvent *event) {
    //Overriding the close event for the route window so that there are no leftover highlighted routes
    for(auto r: tmp) r->clearEdge();
    event->accept();
}

EdgeWindow::EdgeWindow(Edge* e, QGraphicsScene* sc, QUndoStack* undS, QWidget *parent) : QDialog(parent), edge(e), scene(sc), undoStack(undS){
    setFixedSize(140,25);
    setWindowTitle("Insira o peso");
    w = new QLineEdit(this);
    w->setValidator(new QIntValidator(1,9999, this));
    EdgeWindow::connect(w, &QLineEdit::returnPressed, this, [this]() {
        QString l = w->text();
        if(l.isEmpty()) close();
        else {
            undoStack->push(new EditWeightOP(scene,edge->getSource()->getLabel(),edge->getDest()->getLabel(),edge->getWeight(),edge->isDirected()));
            edge->setWeight(l.toInt());
            close();
        }
    });
}

NodeWindow::NodeWindow(Node *n, const QList<Node *> &ns, QUndoStack* undS, QWidget *parent) : QDialog(parent), nodes(ns), node(n), undoStack(undS){
    setFixedSize(135,35);
    setWindowTitle("Insira o rótulo");
    newLabel = new QLineEdit(this);
    error = new QLabel(this);
    error->setStyleSheet("color: red;");
    error->setText("*Rótulo já existe");
    error->setVisible(false);
    error->move(0,20);
    connect(newLabel, &QLineEdit::returnPressed, this, [this]() {
        if(newLabel->text().isEmpty()) {
            close();
        } else {
            bool valid=true;
            for(Node* n: nodes) {
                if(n->getLabel()==newLabel->text()) {
                    if(n==node) close();
                    error->setVisible(true);
                    QTimer::singleShot(2000, error, &QLabel::hide);
                    valid = false;
                    break;
                }
            }
            if(valid) {
                undoStack->push(new EditNodeOP(node->getLabel(), newLabel->text(), nodes));
                node->setLabel(newLabel->text());
                close();
            }
        }
    });
}
