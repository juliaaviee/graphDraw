#include "windows.h"
#include "ui_mainwindow.h"
int n{1}, i{};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , main_ui(new Ui::MainWindow)
{
    main_ui->setupUi(this);
    setWindowTitle("graphDraw");
    setWindowIcon(QIcon(":/graph.png"));
    setFixedSize(799,584);

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

    weightV = main_ui->weightV;
    weightP = main_ui->weightP;
    weightV->setChecked(true);
    weightP->setChecked(true);

    QButtonGroup* sorting = new QButtonGroup(this);
    sorting->addButton(cost);
    sorting->addButton(length);
    length->setChecked(true);

    connectionS = new QShortcut(QKeySequence("Ctrl+Z"), this);
    MainWindow::connect(connectionS, &QShortcut::activated, this, [this]() {
        this->canDelete = false;
        if(this->canConnect==false) this->canConnect = true;
        else this->canConnect = false;
    });

    deletionS = new QShortcut(QKeySequence("Ctrl+X"), this);
    MainWindow::connect(deletionS, &QShortcut::activated, this, [this]() {
        this->canConnect = false;
        if(this->canDelete==false) this->canDelete = true;
        else this->canDelete = false;
    });

    MainWindow::connect(changeName, &QPushButton::clicked, this, [this]() {
        if(cur) {
            QString l = newname->toPlainText();
            if(l.isEmpty()) return;
            for(Node* no: nodes) {
                if(no->getLabel()==l) {
                    qDebug() << "This label already exists";
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
            qDebug() << "Can't calculate routes with missing nodes";
            return;
        }
        if(s==d) {
            qDebug() << "No routes to calculate, source and destination are the same";
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
            qDebug() << "Invalid label(s)";
            return;
        }
        QList<Route> routes;
        Route r;
        traversal(st,f, r, 0, routes,vis,b);
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
        routeWindow = new RouteWindow(routes);
        routeWindow->show();
    });
    MainWindow::connect(showMatrix, &QPushButton::clicked, this, [this]() {
        if(nodes.size()<2) {
            qDebug() << "Not enough nodes to construct a matrix";
            return;
        }
        std::unordered_map<QString, int> idx; //Unordered map is useful for mapping labels to indexes on a string
        std::unordered_map<int, Node*> n_map; //Mapping index to node for connection handling in matrix window
        QList<QPair<QString, QString>> matrix;
        int cnt{};
        for(Node* no: nodes) {
            //Mapping labels to index
            idx[no->getLabel()] = cnt;
            n_map[cnt] = no;
            cnt++;
        }
        for(Node* no: nodes) {
            QString temp(i, '0'); // Declaring string that will translate into row
            temp[idx[no->getLabel()]] = 'X';
            QList<Edge*> es = no->getCons();
            for(Edge* e: es) if(e->getDest()!=no) temp[idx[e->getDest()->getLabel()]] = '1';
            QPair b = qMakePair(no->getLabel(), temp);
            matrix.append(b);
        }
        if(matrixWindow) {matrixWindow->close(); delete matrixWindow;}
        matrixWindow = new MatrixWindow(cnt,cnt,matrix,n_map);
        matrixWindow->show();
        connect(matrixWindow->getModel(), &QStandardItemModel::dataChanged, this, &MainWindow::matrixResponse);
    });
    MainWindow::connect(complete, &QPushButton::clicked, this, [this]() {
        int size = nodes.size();
        for(int i =0;i<size;i++) {
            QList<Edge*> cons = nodes[i]->getCons();
            for(int j = 0;j<size;j++) {
                if(nodes[i]==nodes[j]) continue;
                bool valid=true;
                for(Edge* e: cons) if(e->getSource()==nodes[i]&&e->getDest()==nodes[j]) {valid =false; break;}
                if(valid) {
                    Edge* e = new Edge(nodes[i], nodes[j]);
                    MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
                    nodes[i]->addEdge(e);
                    nodes[j]->addEdge(e);
                    scene->addItem(e);
                    if(matrixWindow) matrixWindow->addCon(e);
                    if(weightV->checkState()==0) e->getLabel()->setVisible(false);
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
        r.edges = backtrack(r.visual,b);
        rs.append(r);
        return;
    }
    v[c] = true; // Set the current node as visited
    r.visual += c->getLabel() + QString::fromStdString("->");
    QList<Edge*> es = c->getCons();
    for(Edge* e: es) {
        Node* ds = e->getDest();
        if(c!=ds) traversal(ds,d,r,sum+e->getWeight(),rs,v,b); // If the destination of the current edge isn't the current node, visit it
    }
}

//The backtrack function was created to ensure we get the correct edges that are to be highlighted, since we had a few issues with traversal.
//It is called when we find our destination in the traversal recursion
QList<Edge*> MainWindow::backtrack(const QString &r, std::unordered_map<QString, Node *> &b) {
    QList<Edge*> ans;
    QStringList t = r.split("->");
    int s = t.size()-1;
    for(int l = 0;l<s;l++) {
        QList<Edge*> re = b[t[l]]->getCons();
        for(auto e: re) {
            if(b[t[l+1]] == e->getDest()) {
                ans.append(e);
                break;
            }
        }
    }
    return ans;
}
void MainWindow::on_insertNode_clicked()
{
    QString l = name->toPlainText();
    if(l.isEmpty()) {
        QString num = QString::number(n);
        for(Node* no: nodes) {
            if(num==no->getLabel()) {
                qDebug()<< "This label already exists";
                return;
            }
        }
        Node *node = new Node(num);
        scene->addItem(node);
        MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
        nodes.append(node);
        n++;
        i++;
    } else {
        for(Node* no: nodes) {
            if(l==no->getLabel()) {
                qDebug() << "This label already exists";
                return;
            }
        }
        Node *node = new Node(l);
        scene->addItem(node);
        MainWindow::connect(node, &Node::selected, this, &MainWindow::nodeInteraction);
        nodes.append(node);
        i++;
    }
}

void MainWindow::nodeInteraction(Node *node) {
    cur = node;
    if(canConnect) {
        if(!tmp) tmp = node;
        else if(tmp != node) {
            QList<Edge*> edges = tmp->getCons();
            for(Edge* e: edges) {
                //This loop ensures this isn't a connection we've made before
                if(tmp == e->getSource() && node == e->getDest()) {
                    tmp = nullptr;
                    return;
                }
            }
            Edge* e = new Edge(tmp, node);
            MainWindow::connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
            tmp->addEdge(e);
            node->addEdge(e);
            scene->addItem(e);
            if(matrixWindow) matrixWindow->addCon(e);
            tmp = nullptr;
            if(weightV->checkState()==0) e->getLabel()->setVisible(false);
            if(weightP->checkState()==2) {
                if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
                edgeWindow = new EdgeWindow(e);
                edgeWindow->show();
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
        return;
    }
    if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
    edgeWindow = new EdgeWindow(e);
    edgeWindow->show();
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
        if(eref[key]) return; //If the connection has already been made, ignore
        //Otherwise, execute proper operations
        Edge* e = new Edge(nref[topLeft.row()], nref[topLeft.column()]);
        nref[topLeft.row()]->addEdge(e);
        nref[topLeft.column()]->addEdge(e);
        eref[key] = e;
        scene->addItem(e);
        connect(e, &Edge::selected, this, &MainWindow::edgeInteraction);
        if(weightV->checkState()==0) e->getLabel()->setVisible(false);
        if(weightP->checkState()==2) {
            if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
            edgeWindow = new EdgeWindow(e);
            edgeWindow->show();
        }
    }
    else
    {
        if(!eref[key]) return; //If there was no connection to begin with, ignore
        nref[topLeft.row()]->removeEdge(eref[key]);
        nref[topLeft.column()]->removeEdge(eref[key]);
        scene->removeItem(eref[key]);
        delete eref[key];
        eref[key] = nullptr;
    }
}
void MainWindow::on_reset_clicked()
{
    if(routeWindow) {routeWindow->close(); delete routeWindow;}
    if(matrixWindow) {matrixWindow->close(); delete matrixWindow;}
    if(edgeWindow) {edgeWindow->close(); delete edgeWindow;}
    scene->clear();
    nodes.clear();
    n = 1;
    i=0;
}

void MainWindow::on_weightV_stateChanged(int b)
{
    if(b==Qt::Checked) {
        for(auto it: scene->items()) {
            Edge* e = qgraphicsitem_cast<Edge*>(it);
            if(e) e->getLabel()->setVisible(true);
        }
    } else {
        for(auto it: scene->items()) {
            Edge* e = qgraphicsitem_cast<Edge*>(it);
            if(e) e->getLabel()->setVisible(false);
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
MatrixWindow::MatrixWindow(int r, int c, const QList<QPair<QString, QString>> &m, const std::unordered_map<int, Node*> &nMap, QWidget *parent) : QDialog(parent), nodeMap(nMap){
    setWindowTitle("Matriz adj.");
    setFixedSize(500,300);
    setWindowIcon(QIcon(":/matrix.png"));
    view = new QTableView(this);
    model = new QStandardItemModel(r,c,this);
    int k{};
    int s = m.size();
    while(k<s) {
        //Setting headers
        model->setHeaderData(k, Qt::Horizontal, m[k].first);
        model->setHeaderData(k, Qt::Vertical, m[k].first);
        for(int co = 0;co<r;co++) {
            //Looping through the row and populating table
            QStandardItem *cur = new QStandardItem(m[k].second[co]);
            if(co==k) cur->setFlags(cur->flags()&~Qt::ItemIsEditable&~Qt::ItemIsSelectable); //If it's part of the diagonal, make it not editable nor selectable
            //If x connects to y, search through x's connections to find the related edge
            if(cur->text() == "1") {
                QList<Edge*> es = nodeMap[k]->getCons();
                for(auto e: es) {
                    if(e->getDest() == nodeMap[co]) {
                        cellCons[std::pair(k,co)] = e;
                        break;
                    }
                }
            }
            cur->setTextAlignment(Qt::AlignCenter);
            model->setItem(k, co, cur);
        }
        k++;
    }
    //We must connect the model data change signal to main window's matrixResponse slot
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

void MatrixWindow::removeCon(Edge *e) {
    for(auto c: cellCons) {
        if(c.second == e) {
            model->setData(model->index(c.first.first, c.first.second), "0", Qt::EditRole);
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
    cellCons[std::make_pair(r,c)] = e;
    model->setData(model->index(r,c), "1", Qt::EditRole);
}
std::unordered_map<std::pair<int,int>, Edge*, PairHash>& MatrixWindow::getCellCons() {
    return cellCons;
}
//The ComboBoxDelegate class was made to change the edit mode for each cell in the table view within MatrixWindow
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

RouteWindow::RouteWindow(const QList<Route> &rs, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Rotas: " + QString::number(rs.length()));
    setFixedSize(500,300);
    list = new QListWidget(this);
    for(Route r: rs) {
        //Mapping routes to their respective edges while populating list
        routes[r.visual] = r.edges;
        list->addItem(r.visual + " Cost: " + QString::number(r.totalCost));
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
