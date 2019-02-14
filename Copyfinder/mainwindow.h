#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include "equals_class.h"
#include <QFile>
#include <QFuture>
#include <QProgressDialog>
#include <future>

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
    bool stop = false;
    QMap<qint64, equals_class*> equals_classes;
    QVector<QFile *> duplicates;
    std::unique_ptr<Ui::MainWindow> ui;
private:
    void split_by_size(QDirIterator && dir_it);
    void split_by_hash(std::vector<QPair<xxh::hash64_t, QFile*>> &files);
    void pul_in_ui(QVector<QFuture<void>> & split);
    void stop_finding();
};

bool check(QFile * first, QFile * second);

#endif // MAINWINDOW_H
