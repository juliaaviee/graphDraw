#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTextEdit>
#include <structures.h>
#include <QShortcut>
#include <QPushButton>
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
    void traversal(Node* c, Node* d, QString r, QList<QString>& rs, std::unordered_map<Node*, bool> v);

private slots:
    void on_insertNode_clicked();
    void nodeInteraction(Node* node);
    void on_reset_clicked();

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QTextEdit *name;
    QTextEdit* newname;
    QTextEdit* src;
    QTextEdit* dst;
    QPushButton* changeName;
    QPushButton* showMatrix;
    QPushButton* showRoutes;
    Node* tmp = nullptr;
    Node* cur = nullptr;
    QList<Node*> nodes;
    bool canConnect=false;
    bool canDelete = false;
    QShortcut* connectionS;
    QShortcut* deletionS;
};

#endif // MAINWINDOW_H
