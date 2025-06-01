#ifndef WINDOWS_H
#define WINDOWS_H

#include <structures.h>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QMainWindow>
#include <QModelIndex>
#include <QDir>
#include <QFileDialog>
#include <QTextEdit>
#include <QShortcut>
#include <QPushButton>
#include <QTableView>
#include <QStandardItemModel>
#include <QDialog>
#include <QPair>
#include <QStringList>
#include <QHeaderView>
#include <QListWidget>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QIcon>
#include <QIntValidator>
#include <QTimer>
#include <QStyledItemDelegate>
#include <unordered_map>
#include <utility> // for std::pair
#include <QUndoStack>
#include <QUndoCommand>
#include <functional> // for std::hash

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MatrixWindow;
class RouteWindow;
class EdgeWindow;
class NodeWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void traversal(Node* c, Node* d, Route r, int sum, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b);
    void traversal (Node* c, Node* d, Route r, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b);
    QList<Edge*> backtrack(const QString &r, std::unordered_map<QString, Node*> &b, bool dir);
    MatrixWindow* getMatrixWindow() {return matrixWindow;}
    RouteWindow* getRouteWindow() {return routeWindow;}
    EdgeWindow* getEdgeWindow() {return edgeWindow;}
    NodeWindow* getNodeWindow() {return nodeWindow;}
    QGraphicsScene* getScene() {return scene;}
    const QList<Node*>& getNodes() {return nodes;}
protected:
    void closeEvent(QCloseEvent *event) override;
public slots:
    void nodeLabelEdit(Node* n);
    void nodeInteraction(Node* node);
    void edgeInteraction(Edge* e);
private slots:
    void on_insertNode_clicked();
    void matrixResponse(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void on_reset_clicked();
    void on_weightV_stateChanged(int b);
    void on_enWeight_stateChanged(int b);
    void on_enDirection_stateChanged(int b);
    void on_showMatrix_clicked();
    void on_save_clicked();
    void on_load_clicked();

private:
    Ui::MainWindow *main_ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    QTextEdit *name;
    QTextEdit* src;
    QTextEdit* dst;
    QPushButton* showMatrix;
    QPushButton* showRoutes;
    QPushButton* complete;
    QRadioButton* cost;
    QRadioButton* length;
    QCheckBox* weightV;
    QCheckBox* weightP;
    CSpinBox* nodeAmount;
    CSpinBox* minWRange;
    CSpinBox* maxWRange;
    Node* tmp = nullptr;
    QList<Node*> nodes;
    bool canConnect=false;
    bool canDelete = false;
    bool directionEnabled = true;
    bool weightEnabled = true;
    QShortcut* connectionS;
    QShortcut* deletionS;
    QShortcut* revert;
    MatrixWindow* matrixWindow=nullptr;
    RouteWindow* routeWindow=nullptr;
    EdgeWindow* edgeWindow=nullptr;
    NodeWindow* nodeWindow=nullptr;
    QUndoStack* undoStack = new QUndoStack(this);
};

//This struct was made so that we can map table coordinates to a specific edge
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const noexcept {
        auto h1 = std::hash<int>{}(p.first);
        auto h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 1); // or use another combiner like boost::hash_combine
    }
};

class MatrixWindow: public QDialog
{
    Q_OBJECT
public:
    MatrixWindow(unsigned short r, unsigned short c, const QList<QPair<QString, QString>> &m, const std::unordered_map<int, Node*> &nMap, bool dir, QWidget *parent = nullptr);
    std::unordered_map<int, Node*>& getNodeMap();
    std::unordered_map<std::pair<int,int>, Edge*, PairHash>& getCellCons();
    QStandardItemModel* getModel();
    void toggleDirection(bool dir);
    void removeCon(Edge* e);
    void addCon(Edge* e);
private:
    QTableView *view;
    QStandardItemModel *model;
    std::unordered_map<int, Node*> nodeMap;
    std::unordered_map<std::pair<int,int>, Edge*, PairHash> cellCons;
    bool directed = true;
};

class ComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ComboBoxDelegate(const QStringList &options, QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
private:
    QStringList m_options;
};

class EdgeWindow : public QDialog
{
    Q_OBJECT
public:
    EdgeWindow(Edge* e, QGraphicsScene* sc, QUndoStack* undS, QWidget* parent = nullptr);
private:
    QLineEdit* w;
    Edge* edge;
    QGraphicsScene* scene;
    QUndoStack* undoStack;
};

class NodeWindow: public QDialog {
    Q_OBJECT
public:
    NodeWindow(Node* n, const QList<Node*>& ns, QUndoStack* undS,QWidget* parent = nullptr);
private:
    QLineEdit* newLabel;
    QLabel* error;
    const QList<Node*>& nodes;
    Node* node;
    QUndoStack* undoStack;
};

class RouteWindow : public QDialog
{
    Q_OBJECT
public:
    RouteWindow(const QList<Route> &rs, bool w, QWidget *parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    std::unordered_map<QString, QList<Edge*>> routes;
    QList<Edge*> tmp;
    QListWidget *list;
    bool weighted;
};

class AddNodeOP : public QUndoCommand {
public:
    AddNodeOP(QGraphicsScene* scene, QList<QString> l, QList<Node*>& ns, short unsigned int& I, MainWindow* mw) : labels(l), scene(scene), nodes(ns), mW(mw),i(I){}
    void undo() override {
        for(QString l : labels){
            for(auto it: scene->items()) {
                Node* n = qgraphicsitem_cast<Node*>(it);
                if(n) {
                    if(n->getLabel()==l) {
                        scene->removeItem(n);
                        nodes.removeAt(nodes.lastIndexOf(n));
                        delete n;
                        i--;
                    }
                }
            }
        }
        if(mW->getMatrixWindow()!=nullptr) {
            MatrixWindow* mtw = mW->getMatrixWindow();
            mtw->close();
        }
    }
private:
    QList<QString> labels;
    QGraphicsScene* scene;
    QList<Node*>& nodes;
    MainWindow* mW;
    short unsigned int& i;
};

struct EdgeInfo { // This struct was made to simplify rolling back a node removal (storing info on all edges that were deleted alongside it [if any])
    std::pair<QString,QString> mainE;
    bool ctrpart;
    int edgeW;
    int ctrpartW;
};

class RemoveNodeOP : public QUndoCommand {
public:
    RemoveNodeOP(QGraphicsScene* scene, Node* n, QPointF p, QList<Node*>& ns, short unsigned int& I, bool dir, bool wvis, MainWindow* mw) : label(n->getLabel()), Pos(p),scene(scene), nodes(ns), i(I), directed(dir), vis(wvis), mW(mw){
        std::unordered_map<Edge*,bool> stored;
        for(Edge* e : n->getCons()) {
            if(!stored[e]){
                EdgeInfo info;
                info.mainE.first = e->getSource()->getLabel();
                info.mainE.second = e->getDest()->getLabel();
                info.edgeW = e->getWeight();
                if(e->getCounterpart()) { // If there is a counterpart, store its info alongside the other edge, while also marking it so we don't mirror it
                    info.ctrpart = true;
                    info.ctrpartW = e->getCounterpart()->getWeight();
                    stored[e->getCounterpart()] = true;
                }
                edges.append(info);
            }
        }
    }
    void undo() override {
        Node* node = new Node(label);
        MainWindow::connect(node, &Node::selected, mW, &MainWindow::nodeInteraction);
        MainWindow::connect(node, &Node::edit, mW, &MainWindow::nodeLabelEdit);
        nodes.append(node);
        node->setPos(Pos);
        scene->addItem(node);
        i++;
        for(EdgeInfo e: edges) {
            Node* src=nullptr;
            Node* dst=nullptr;
            for(Node* n: mW->getNodes()) {
                if(n->getLabel()==e.mainE.first) src = n;
                else if(n->getLabel()==e.mainE.second) dst = n;
                if(src&&dst) break; // If both src and dst were found, break out
            }
            if(src&&dst) {
                Edge* E = new Edge(src,dst,e.edgeW);
                MainWindow::connect(E, &Edge::selected, mW, &MainWindow::edgeInteraction);
                E->directionToggle(directed);
                E->getLabel()->setVisible(vis);
                src->addEdge(E);
                dst->addEdge(E);
                scene->addItem(E);
                if(e.ctrpart) {
                    Edge* NE = new Edge(dst,src,e.ctrpartW);
                    MainWindow::connect(NE, &Edge::selected, mW, &MainWindow::edgeInteraction);
                    E->setCounterpart(NE);
                    NE->setCounterpart(E);
                    NE->getLabel()->setVisible(vis);
                    src->addEdge(NE);
                    dst->addEdge(NE);
                    scene->addItem(NE);
                }
            }
        }
        MatrixWindow* mtw = mW->getMatrixWindow();
        if(mtw) mtw->close();
    }
private:
    QString label;
    QPointF Pos;
    QGraphicsScene* scene;
    QList<Node*>& nodes;
    QList<EdgeInfo> edges;
    short unsigned int& i;
    bool directed;
    bool vis;
    MainWindow* mW; //Storing main window pointer to get access to related slots
};

class MakeConnectionOP : public QUndoCommand {
public:
    MakeConnectionOP(QGraphicsScene* s, QString src, QString dst, bool dir, bool w, MainWindow* mw) : scene(s),source(src), dest(dst), directed(dir), weighted(w), mW(mw){}
    void undo() override {
        for(QGraphicsItem* it: scene->items()) {
            Edge* e = qgraphicsitem_cast<Edge*>(it);
            if(e) {
                if(directed) {
                    if(e->getSource()->getLabel()==source&&e->getDest()->getLabel()==dest) {
                        MatrixWindow* mtw = mW->getMatrixWindow();
                        if(mtw) {mtw->removeCon(e); return;}
                        e->getSource()->removeEdge(e);
                        e->getDest()->removeEdge(e);
                        if(e->getCounterpart()) e->getCounterpart()->setCounterpart(nullptr);
                        scene->removeItem(e);
                        EdgeWindow* ew = mW->getEdgeWindow();
                        if(ew) ew->close();
                    }
                } else {
                    if((e->getSource()->getLabel()==source&&e->getDest()->getLabel()==dest) || (e->getDest()->getLabel()==source&&e->getSource()->getLabel()==dest)) {
                        MatrixWindow* mtw = mW->getMatrixWindow();
                        if(mtw) {mtw->removeCon(e); return;}
                        e->getSource()->removeEdge(e);
                        e->getDest()->removeEdge(e);
                        scene->removeItem(e);
                        EdgeWindow* ew = mW->getEdgeWindow();
                        if(ew) ew->close();
                    }
                }
            }
        }
    }
private:
    QGraphicsScene* scene;
    QString source, dest;
    bool directed;
    bool weighted;
    MainWindow* mW;
};

class RemoveConnectionOP : public QUndoCommand {
public:
    RemoveConnectionOP(QGraphicsScene* s, QString src, QString dst, int wt, bool dir, bool w, bool ctr,MainWindow* mw) : scene(s),source(src), dest(dst), directed(dir), weighted(w), counter(ctr), weight(wt), mW(mw){}
    void undo() override {
        Node* src=nullptr;
        Node* dst=nullptr;
        for(Node* n: mW->getNodes()) {
            if(n->getLabel()==source) src = n;
            else if(n->getLabel()==dest) dst = n;
            if(src&&dst) break;
        }
        if(src&&dst) {
            Edge* e = new Edge(src,dst, weight);
            e->directionToggle(directed);
            MainWindow::connect(e, &Edge::selected, mW, &MainWindow::edgeInteraction);
            e->getLabel()->setVisible(weighted);
            src->addEdge(e);
            dst->addEdge(e);
            scene->addItem(e);
            MatrixWindow* mtw = mW->getMatrixWindow();
            if(mtw) mtw->addCon(e);
            if(counter) {
                for(auto it: scene->items()) {
                    Edge* edge = qgraphicsitem_cast<Edge*>(it);
                    if(edge) {
                        if(edge->getSource()==dst&&edge->getDest()==src) {
                            e->setCounterpart(edge);
                            edge->setCounterpart(e);
                            edge->getLabel()->setVisible(weighted);
                            break;
                        }
                    }
                }
            }
        }
    }
private:
    QGraphicsScene* scene;
    QString source, dest;
    bool directed;
    bool weighted;
    bool counter;
    int weight;
    MainWindow* mW;
};

class EditNodeOP : public QUndoCommand {
public:
    EditNodeOP(QString oldL, QString curL, const QList<Node*>& ns) : oldLabel(oldL), curLabel(curL), nds(ns){}
    void undo() override {
        for(Node* n: nds) if(n->getLabel()==curLabel) {n->setLabel(oldLabel); break;}
    }
private:
    QString oldLabel, curLabel;
    const QList<Node*>& nds;
};

class EditWeightOP : public QUndoCommand {
public:
    EditWeightOP(QGraphicsScene* scene, QString src, QString dst, int oldW, bool dir) : scene(scene), source(src), dest(dst), weight(oldW),directed(dir) {}
    void undo() override {
        for(auto it: scene->items()) {
            Edge* e = qgraphicsitem_cast<Edge*>(it);
            if(e) {
                if(directed) {
                    if(e->getSource()->getLabel()==source&&e->getDest()->getLabel()==dest) {e->setWeight(weight); break;}
                } else {
                    if((e->getSource()->getLabel()==source&&e->getDest()->getLabel()==dest)||(e->getDest()->getLabel()==source&&e->getSource()->getLabel()==dest)) {e->setWeight(weight); break;}
                }
            }
        }
    }
private:
    QGraphicsScene* scene;
    QString source, dest;
    int weight;
    bool directed;
};

#endif // WINDOWS_H
