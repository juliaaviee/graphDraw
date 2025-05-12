#ifndef WINDOWS_H
#define WINDOWS_H

#include <structures.h>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QMainWindow>
#include <QModelIndex>
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
#include <QRadioButton>
#include <QButtonGroup>
#include <QIcon>
#include <QIntValidator>
#include <QTimer>
#include <QStyledItemDelegate>
#include <unordered_map>
#include <utility> // for std::pair
#include <functional> // for std::hash

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MatrixWindow;
class RouteWindow;
class EdgeWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void traversal(Node* c, Node* d, Route r, int sum, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b);
    void traversal (Node* c, Node* d, Route r, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b);
    QList<Edge*> backtrack(const QString &r, std::unordered_map<QString, Node*> &b, bool dir);
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_insertNode_clicked();
    void nodeInteraction(Node* node);
    void edgeInteraction(Edge* e);
    void matrixResponse(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void on_reset_clicked();
    void on_weightV_stateChanged(int b);
    void on_enWeight_stateChanged(int b);
    void on_enDirection_stateChanged(int b);
    void on_showMatrix_clicked();

private:
    Ui::MainWindow *main_ui;
    QGraphicsScene *scene;
    QTextEdit *name;
    QTextEdit* newname;
    QTextEdit* src;
    QTextEdit* dst;
    QPushButton* changeName;
    QPushButton* showMatrix;
    QPushButton* showRoutes;
    QPushButton* complete;
    QRadioButton* cost;
    QRadioButton* length;
    QCheckBox* weightV;
    QCheckBox* weightP;
    CSpinBox* nodeAmount;
    Node* tmp = nullptr;
    Node* cur = nullptr;
    QList<Node*> nodes;
    bool canConnect=false;
    bool canDelete = false;
    bool directionEnabled = true;
    bool weightEnabled = true;
    QShortcut* connectionS;
    QShortcut* deletionS;
    MatrixWindow* matrixWindow=nullptr;
    RouteWindow* routeWindow=nullptr;
    EdgeWindow* edgeWindow=nullptr;
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
    EdgeWindow(Edge* e, QWidget* parent = nullptr);
private:
    QLineEdit* w;
    Edge* edge=nullptr;
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

#endif // WINDOWS_H
