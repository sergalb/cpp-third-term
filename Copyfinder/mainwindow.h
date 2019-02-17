#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include "equals_class.h"
#include <QFile>
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
    void take_part_duplicates(QVector<QVector<QFile *>> * duplicates);

private:
    QVector<QFile*> duplicates;
    //QVector<QFile*> exceptions_files;
    std::unique_ptr<Ui::MainWindow> ui;
private:
};



#endif // MAINWINDOW_H
