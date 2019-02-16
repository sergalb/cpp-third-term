#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <thread>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <future>
#include <functional>
#include <QException>
#include <QThread>
main_window::main_window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionFind_copy->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    qRegisterMetaType<QVector<int>>("QVector<int>");
    connect(ui->actionFind_copy, &QAction::triggered, this, &main_window::select_directory_and_scan);
}

void main_window::select_directory_and_scan()
{

    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for find copy",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    scanner* scan = new scanner(dir);
    QThread* scan_thread = new QThread();
    scan->moveToThread(scan_thread);
    connect(scan_thread, &QThread::started, scan, &scanner::scan_directory);
    connect(scan, &scanner::return_duplicates, this, &main_window::take_duplicates);
    connect(scan, &scanner::finished, this, &main_window::pull_in_ui);
    connect(scan, &scanner::finished, scan_thread, &QThread::quit);
    connect(scan_thread, &QThread::finished, scan, &scanner::deleteLater);
    connect(scan_thread, &QThread::finished, scan_thread, &QThread::deleteLater);
    scan_thread->start();
}

void main_window::pull_in_ui()
{
    QMessageBox message;
    message.setText("scanning is finished");
    message.exec();
    for (int j = 0; j < duplicates->size(); ++j) {
        QFileInfo file_info(*(duplicates->operator[](j)));
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, file_info.fileName());
        item->setText(1, QString::number(duplicates->size()));
        item->setText(2, QString::number(file_info.size()));
        ui->treeWidget->addTopLevelItem(item);
        //delete item;
    }
    QCoreApplication::processEvents();
}

void main_window::take_duplicates(QVector<QFile *> *duplicates)
{
    this->duplicates = duplicates;
}

main_window::~main_window() = default;
