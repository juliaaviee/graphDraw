#include "windows.h"
#include "ui_mainwindow.h"
#include <QTextStream>


int n{1}, i{};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , main_ui(new Ui::MainWindow)
{
    main_ui->setupUi(this);
    scene = new QGraphicsScene(this);
    main_ui->graphicsView->setScene(scene);
    setWindowTitle("graphDraw");
    setFixedSize(799,584);
    name = main_ui->nodeName;
    newname = main_ui->newName;
    src = main_ui->src;
    dst = main_ui->dst;

    changeName = main_ui->changeName;
    showRoutes = main_ui->routes;
    showMatrix = main_ui->showMatrix;

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
            if(no->getLabel()==s) {
                st = no;
            }
            else if(no->getLabel()==d) {
                f = no;
            }
            vis[no] = false;
            b[no->getLabel()] = no;
        }
        if(!st||!f) {
            qDebug() << "Invalid label(s)";
            return;
        }
        QList<QPair<QString, QList<Edge*>>> routes;
        traversal(st,f, "", routes,vis,b);
        if(routes.empty()) {
            qDebug() << "No possible routes from" << s << "to" << d;
            return;
        }
        std::sort(routes.begin(), routes.end(), [](const QPair<QString, QList<Edge*>> &a, const QPair<QString, QList<Edge*>> &b) {
            return a.first.size() < b.first.size();  // sort by length
        });
        if(routeWindow) routeWindow->close();
        routeWindow = new RouteWindow(routes);
        routeWindow->show();
    });
    MainWindow::connect(showMatrix, &QPushButton::clicked, this, [this]() {
        if(nodes.size()<2) {
            qDebug() << "Not enough nodes to construct a matrix";
            return;
        }
        std::unordered_map<QString, int> idx; //Unordered map is useful for mapping labels to indexes on a string
        QList<QPair<QString, QString>> matrix;
        int cnt{};
        for(Node* no: nodes) {
            //Printing out columns while mapping labels to index
            idx[no->getLabel()] = cnt;
            cnt++;
        }
        for(Node* no: nodes) {
            QString temp(i, '0'); // Declaring string that will translate into row
            temp[idx[no->getLabel()]] = 'X';
            QList<Edge*> es = no->getCons();
            for(Edge* e: es) {
                if(e->getSource()!=no){
                    temp[idx[e->getSource()->getLabel()]] = '1';
                } else if(e->getDest()!=no) {
                    temp[idx[e->getDest()->getLabel()]] = '1';
                }
            }
            QPair b = qMakePair(no->getLabel(), temp);
            matrix.append(b);
        }
        if(matrixWindow) matrixWindow->close();
        matrixWindow = new MatrixWindow(cnt,cnt,matrix);
        matrixWindow->show();
    });
}

MainWindow::~MainWindow()
{
    delete main_ui;
}

void MainWindow::traversal(Node *c, Node *d, QString r, QList<QPair<QString, QList<Edge*>>> &rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> b) {
    if(v[c]) return; // If it has been visited already, return
    if(c==d) {
        // If we have arrived at our destination, concatenate it to the end of the route and append it to routes, then return
        r += c->getLabel();
        QPair<QString, QList<Edge*>> p = qMakePair(r, backtrack(r, b));
        rs.append(p);
        return;
    }
    v[c] = true; // Set the current node as visited
    r += c->getLabel() + QString::fromStdString("->");
    QList<Edge*> es = c->getCons();
    for(Edge* e: es) {
        Node* s = e->getSource();
        Node* ds = e->getDest();
        if(c!=s) traversal(s,d,r,rs,v,b);
        else traversal(ds,d,r,rs,v,b);
    }
}

//The backtrack function was created to ensure we get the correct edges that are to be highlighted, since we had a few issues with traversal.
//It is called when we find our destination in the traversal recursion
QList<Edge*> MainWindow::backtrack(const QString &r, std::unordered_map<QString, Node *> b) {
    QList<Edge*> ans;
    QStringList t = r.split("->");
    int s = t.size()-1;
    for(int l = 0;l<s;l++) {
        QList<Edge*> re = b[t[l]]->getCons();
        for(auto e: re) {
            if((b[t[l]] == e->getSource() && b[t[l+1]] == e->getDest()) || (b[t[l]] == e->getDest() && b[t[l+1]] == e->getSource())) {
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
            QList<Edge*> edges = node->getCons();
            for(Edge* n: edges) {
                Node* s = n->getSource();
                Node* d = n->getDest();
                if((tmp == s && node == d) || (node == s && tmp == d)) {
                    tmp = nullptr;
                    return;
                }
            }
            Edge* e = new Edge(tmp, node);
            tmp->addEdge(e);
            node->addEdge(e);
            scene->addItem(e);
            tmp = nullptr;
        }
    }
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
        }
        scene->removeItem(node);
        nodes.removeAt(nodes.lastIndexOf(node));
        i--;
    }
}

void MainWindow::on_reset_clicked()
{
    if(routeWindow) routeWindow->close();
    if(matrixWindow) matrixWindow->close();
    scene->clear();
    nodes.clear();
    n = 1;
    i=0;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    //Ensuring children windows are closed
    if(routeWindow) routeWindow->close();
    if(matrixWindow) matrixWindow->close();
    event->accept();
}
MatrixWindow::MatrixWindow(int r, int c, const QList<QPair<QString, QString>> &m, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Adj. Matrix");
    setFixedSize(500,300);
    view = new QTableView(this);
    model = new QStandardItemModel(r,c);
    int k{};
    for(QPair<QString, QString> a: m) {
        //Setting headers
        model->setHeaderData(k, Qt::Horizontal, a.first);
        model->setHeaderData(k, Qt::Vertical, a.first);

        for(int co = 0;co<r;co++) {
            //Looping through the row and populating table
            QStandardItem *cur = new QStandardItem(a.second[co]);
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

    //Loop that makes diagonal not editable nor selectable
    for(int cou=0;cou<r;cou++) {
        QStandardItem* it = model->item(cou,cou);
        it->setFlags(it->flags()&~Qt::ItemIsEditable&~Qt::ItemIsSelectable);
    }
}

void MatrixWindow::changeModel(int r, int c) {
    model = new QStandardItemModel(r,c);
    view->setModel(model);
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

RouteWindow::RouteWindow(const QList<QPair<QString, QList<Edge*>>> &rs, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Routes: " + QString::number(rs.length()));
    setFixedSize(500,300);
    list = new QListWidget(this);
    for(auto r: rs) {
        //Mapping routes to their respective edges while populating list
        routes[r.first] = r.second;
        list->addItem(r.first);
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

