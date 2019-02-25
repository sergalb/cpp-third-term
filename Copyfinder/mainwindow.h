#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include "equals_class.h"
#include <QFile>
#include "scanner.h"
#include <QTreeWidgetItem>
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
    void take_part_duplicates(QVector<QVector<QFile *>> * duplicates);
    void stop();
    void delete_duplicates();
    void choose_deleted(QTreeWidgetItem * item);

signals:
    void stop_scan();

private:
    scanner *scan;
    std::unique_ptr<Ui::MainWindow> ui;
    int count_deleted = 0;
private:
};



#endif // MAINWINDOW_H
