#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
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
    connect(scan, &scanner::return_part_duplicates, this, &main_window::take_part_duplicates);
    connect(scan, &scanner::finished, this, &main_window::scanning_finished);
    connect(scan, &scanner::finished, scan_thread, &QThread::quit);
    connect(scan_thread, &QThread::finished, scan, &scanner::deleteLater);
    connect(scan_thread, &QThread::finished, scan_thread, &QThread::deleteLater);
    scan_thread->start();
}

void main_window::scanning_finished()
{

    QMessageBox message;
    message.setText("scanning is finished, count duplicates" +  QString::number(duplicates.size()));
    message.exec();

}

void main_window::take_part_duplicates(QVector<QVector<QFile *>>  *duplicates)
{
    for (QVector<QFile*> & group : *duplicates) {
        if (group.size() <= 1) continue;
        for (int j = 0; j < group.size(); ++j) {
            QFileInfo file_info(*group[j]);
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            //std::cout << file_info.fileName().toStdString() << std::endl;
            item->setText(0, file_info.fileName());
            item->setText(1, QString::number(group.size()));
            item->setText(2, QString::number(file_info.size()));
            ui->treeWidget->addTopLevelItem(item);
            //delete item;
            this->duplicates.push_back(group[j]);
        }
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, " ");
        item->setText(1, " ");
        item->setText(2, " ");
        ui->treeWidget->addTopLevelItem(item);

    }
    //QCoreApplication::processEvents();
    delete duplicates;
}

main_window::~main_window() = default;
