#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextStream>


int n{1}, i{};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    name = ui->nodeName;
    newname = ui->newName;
    src = ui->src;
    dst = ui->dst;

    changeName = ui->changeName;
    showRoutes = ui->routes;
    showMatrix = ui->showMatrix;

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
        for(Node* no: nodes) {
            if(no->getLabel()==s) {
                st = no;
            }
            else if(no->getLabel()==d) {
                f = no;
            }
            vis[no] = false;
        }
        if(!st||!f) {
            qDebug() << "Invalid label(s)";
            return;
        }
        QList<QString> routes;
        traversal(st,f, "", routes, vis);
        if(routes.isEmpty()) {
            qDebug() << "No possible routes from " << s << " to " << d;
            return;
        }
        QTextStream out(stdout);
        out << "All routes:" << Qt::endl;
        for(QString r: routes) {
            out << r << Qt::endl;
        }
    });
    MainWindow::connect(showMatrix, &QPushButton::clicked, this, [this]() {
        if(nodes.size()<2) {
            qDebug() << "Not enough nodes to construct a matrix";
            return;
        }
        std::unordered_map<QString, int> idx; //Unordered map is useful for mapping labels to indexes on a string
        QTextStream out(stdout);
        out << " ";
        int cnt{};
        for(Node* no: nodes) {
            //Printing out columns while mapping labels to index
            out << " " << no->getLabel();
            idx[no->getLabel()] = cnt;
            cnt++;
        }
        out << Qt::endl;
        for(Node* no: nodes) {
            QString temp(i, '0'); // Declaring string that will translate into row
            out << no->getLabel();
            temp[idx[no->getLabel()]] = 'X';
            QList<Edge*> es = no->getCons();
            for(Edge* e: es) {
                if(e->getSource()!=no){
                    temp[idx[e->getSource()->getLabel()]] = '1';
                } else if(e->getDest()!=no) {
                    temp[idx[e->getDest()->getLabel()]] = '1';
                }
            }
            for(QChar c: temp) {
                out << " " << c;
            }
            out << Qt::endl;
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::traversal(Node *c, Node *d, QString r, QList<QString> &rs, std::unordered_map<Node*, bool> v) {
    if(v[c]) return;
    if(c==d) {
        r += c->getLabel();
        rs.append(r);
        return;
    }
    v[c] = true;
    r += c->getLabel() + QString::fromStdString("->");
    QList<Edge*> es = c->getCons();
    for(Edge* e: es) {
        Node* s = e->getSource();
        Node* ds = e->getDest();
        if(c!=s) traversal(s,d,r,rs,v);
        else traversal(ds,d,r,rs,v);
    }
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
    scene->clear();
    nodes.clear();
    n = 1;
    i=0;
}

