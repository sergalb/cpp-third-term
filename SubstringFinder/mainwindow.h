#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include <QFile>
#include <QTreeWidgetItem>
#include "scanner.h"
namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();


private slots:
    void select_directory_and_scan();
    void scanning_finished();
    void stop();
    void pull_in_ui(QVector<QPair<int, QFile*>> * contains_templ);

signals:
    void stop_scan();

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<scanner> scan;
private:
};



#endif // MAINWINDOW_H
