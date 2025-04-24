#ifndef WINDOWS_H
#define WINDOWS_H

#include <structures.h>
#include <QCheckBox>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QTextEdit>
#include <QLineEdit>
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
#include <QCloseEvent>
#include <QComboBox>
#include <QIcon>
#include <QIntValidator>
#include <QStyledItemDelegate>
#include <unordered_map>


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void traversal(Node* c, Node* d, Route r, int sum, QList<Route>& rs, std::unordered_map<Node*, bool> v, std::unordered_map<QString, Node*> &b);
    QList<Edge*> backtrack(const QString &r, std::unordered_map<QString, Node*> &b);
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_insertNode_clicked();
    void nodeInteraction(Node* node);
    void edgeInteraction(Edge* e);
    void on_reset_clicked();
    void on_weightV_stateChanged(int arg1);

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
    QRadioButton* cost;
    QRadioButton* length;
    QCheckBox* weightV;
    QCheckBox* weightP;
    Node* tmp = nullptr;
    Node* cur = nullptr;
    QList<Node*> nodes;
    bool canConnect=false;
    bool canDelete = false;
    QShortcut* connectionS;
    QShortcut* deletionS;
    QDialog* matrixWindow=nullptr;
    QDialog* routeWindow=nullptr;
    QDialog* edgeWindow=nullptr;
};

class MatrixWindow: public QDialog
{
    Q_OBJECT
public:
    MatrixWindow(int r, int c, const QList<QPair<QString, QString>> &m, QWidget *parent = nullptr);
    void changeModel(int r, int c);
private:
    QTableView *view;
    QStandardItemModel *model;
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
    RouteWindow(const QList<Route> &rs, QWidget *parent = nullptr);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    std::unordered_map<QString, QList<Edge*>> routes;
    QList<Edge*> tmp;
    QListWidget *list;
};

#endif // WINDOWS_H
