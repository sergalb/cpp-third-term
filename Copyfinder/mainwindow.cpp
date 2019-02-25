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
    connect(ui->treeWidget, &QTreeWidget::itemActivated, this, &main_window::choose_deleted);
    connect(ui->actionDelete, &QAction::triggered, this, &main_window::delete_duplicates);
}

void main_window::select_directory_and_scan()
{

    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for find copy",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    scan = new scanner(dir);
    QThread* scan_thread = new QThread();
    scan->moveToThread(scan_thread);
    connect(scan_thread, &QThread::started, scan, &scanner::scan_directory);
    connect(scan, &scanner::return_part_duplicates, this, &main_window::take_part_duplicates);
    connect(scan, &scanner::finished, this, &main_window::scanning_finished);
    connect(scan, &scanner::finished, scan_thread, &QThread::quit);
    connect(scan_thread, &QThread::finished, scan, &scanner::deleteLater);
    connect(scan_thread, &QThread::finished, scan_thread, &QThread::deleteLater);
    connect(scan, &scanner::stoped, scan_thread, &QThread::quit);
    connect(this, &main_window::stop_scan, scan, &scanner::stop);
    scan_thread->start();
}

void main_window::scanning_finished()
{
    QMessageBox message;
    message.setText("scanning is finished ");
    message.exec();
}

void main_window::take_part_duplicates(QVector<QVector<QFile *>>  * duplicates)
{
    for (QVector<QFile*> & group : *duplicates) {
        if (group.size() <= 1) continue;
        for (int j = 0; j < group.size(); ++j) {
            QFileInfo file_info(*group[j]);
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            item->setText(0, file_info.fileName());
            item->setText(1, file_info.absoluteFilePath());
            item->setText(2, QString::number(file_info.size()));
            item->setSelected(false);
            ui->treeWidget->addTopLevelItem(item);
        }
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, " ");
        item->setText(1, " ");
        item->setText(2, " ");
        ui->treeWidget->addTopLevelItem(item);

    }
    delete duplicates;
}

void main_window::stop()
{
    emit stop_scan();
}

void main_window::choose_deleted(QTreeWidgetItem *item)
{
    if (item->text(2)==" ") {
        return;
    }
    QColor color = (item->textColor(0) == Qt::black ) ? Qt::blue : Qt ::black;
    if (item->textColor(0) == Qt::blue) {
        --count_deleted;
        color = Qt::black;
    } else {
        ++count_deleted;
        color = Qt::blue;
    }
    item->setTextColor(0, color);
}

void main_window::delete_duplicates() {
    if(QMessageBox::question(this, "Delete duplicates", "Selected files will be deleted, continue?") == QMessageBox::Yes) {
        QTreeWidgetItemIterator it(ui->treeWidget);
        int real_deleted = 0;

        while (count_deleted > 0 && *it) {
            --count_deleted;
            if ((*it)->text(2) != " " && (*it)->textColor(0) == Qt::blue) {
                QFile file((*it)->text(1));
                if (!file.remove()) {
                    QMessageBox::information(this, "Error", "can't delete file " + (*it)->text(1));
                } else {
                    ++real_deleted;
                   (*it)->setHidden(true);
                }
            }
            ++it;
        }
        QMessageBox::information(this, "Delete duplicates", QString::number(real_deleted) + " duplicates are deleted");
    }
}

main_window::~main_window() = default;
