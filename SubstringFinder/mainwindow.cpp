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
    QCommonStyle style;
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->actionFind_copy->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    qRegisterMetaType<QVector<int>>("QVector<int>");
    connect(ui->actionFind_copy, &QAction::triggered, this, &main_window::select_directory_and_scan);
    connect(ui->actionStop, &QAction::triggered, this, &main_window::stop);   
}

void main_window::select_directory_and_scan()
{

    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for find copy",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    scan.reset(new scanner(dir));
    QThread *scan_thread = new QThread();
    scan->moveToThread(scan_thread);
    connect(scan_thread, &QThread::started, scan.get(), &scanner::scan_directory);
    connect(scan.get(), &scanner::substr_finded, this, &main_window::pull_in_ui);
    connect(scan.get(), &scanner::finished, this, &main_window::scanning_finished);
    connect(scan.get(), &scanner::finished, scan_thread, &QThread::quit);
    connect(scan_thread, &QThread::finished, scan_thread, &QThread::deleteLater);
    connect(scan.get(), &scanner::stoped, scan_thread, &QThread::quit);
    connect(this, &main_window::stop_scan, scan.get(), &scanner::stop);
    scan_thread->start();
}

void main_window::scanning_finished()
{
    QMessageBox message;
    message.setText("indexing is finished ");
    message.exec();
}

void main_window::stop()
{
    emit stop_scan();
}

void main_window::pull_in_ui(QVector<QPair<int, QFile*> > * contains_templ)
{   QMessageBox::information(this, "entries", "end search");
    for (QPair<int, QFile*> & entry: *contains_templ) {
        QFileInfo file_info(*entry.second);
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, file_info.fileName());
        item->setText(1, file_info.absoluteFilePath());
        item->setText(2, QString::number(entry.first));
        ui->treeWidget->addTopLevelItem(item);

    }
    delete contains_templ;
}


main_window::~main_window() {
}
