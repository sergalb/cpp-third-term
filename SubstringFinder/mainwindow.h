#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <QMap>
#include <QDirIterator>
#include <QFile>
#include <QTreeWidgetItem>
#include "scanner.h"
#include <QProgressDialog>
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
    void indexing_finished();
    void pull_in_ui(QVector<QPair<int, QString const>> * contains_templ);
    void search_text();
    void stop_index();
    void stop_search();

signals:
    void new_text(std::string const text);

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<scanner> scan;
    std::unique_ptr<QThread> scan_thread;
    std::unique_ptr<QProgressDialog> progressDialog;
private:
};



#endif // MAINWINDOW_H
