#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include "equals_class.h"
#include <QFile>
#include <thread>
#include <mutex>

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
    void select_directory();
    void scan_directory(QString const& dir);

private:
    std::mutex ec_mutex;
    QMap<qint64, equals_class*> equals_classes;
    std::unique_ptr<Ui::MainWindow> ui;
private:
    void split_by_size(QMap<qint64, equals_class*> & equals_classes, QDirIterator && dir_it);
    std::vector<QFile*> & split_by_hash(std::vector<QPair<xxh::hash64_t, QFile*>> &files);
};

bool check(QFile * first, QFile * second);

#endif // MAINWINDOW_H
