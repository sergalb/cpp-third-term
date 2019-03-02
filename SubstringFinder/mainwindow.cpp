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
#include <QPlainTextEdit>
#include <QDesktopServices>

main_window::main_window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), scan_thread(nullptr)
{
    ui->setupUi(this);
    QCommonStyle style;
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->actionIndexing->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<std::string>("std::string");

    connect(ui->actionIndexing, &QAction::triggered, this, &main_window::select_directory_and_scan);
}

void main_window::select_directory_and_scan()
{
    if (scan_thread.get()) scan_thread->quit();
    disconnect(ui->lineEdit, &QLineEdit::editingFinished, this, &main_window::search_text);
    ui->lineEdit->setVisible(false);
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for find copy",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    progressDialog.reset(new QProgressDialog("Indexing directory", "Cancel", 0, 100));
    ui->actionIndexing->setVisible(false);

    scan.reset(new scanner(dir));
    scan_thread.reset(new QThread());
    scan->moveToThread(scan_thread.get());

    connect(scan.get(), &scanner::substr_finded, this, &main_window::pull_in_ui);
    connect(progressDialog.get(), &QProgressDialog::canceled, this, &main_window::stop_index);
    connect(progressDialog.get(), &QProgressDialog::finished,  this, &main_window::indexing_finished);
    connect(scan.get(), &scanner::finish_index, this, &main_window::indexing_finished);

    connect(scan.get(), &scanner::update_progress, progressDialog.get(), &QProgressDialog::setValue);

    connect(scan_thread.get(), &QThread::started, scan.get(), &scanner::scan_directory);
    scan_thread->start();
}

void main_window::indexing_finished()
{
    QMessageBox::information(this, "indexing", "indexing is finished");
    ui->actionIndexing->setVisible(true);
    ui->lineEdit->setVisible(true);

    disconnect(scan.get(), &scanner::update_progress, progressDialog.get(), &QProgressDialog::setValue);

    connect(ui->lineEdit, &QLineEdit::editingFinished, this, &main_window::search_text);
    connect(this, &main_window::new_text, scan.get(), &scanner::find_in_all_files);
}

void main_window::pull_in_ui(QVector<QPair<int, QString const>> * contains_templ){
    for (auto & entry: *contains_templ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, entry.second);
        item->setText(1, QString::number(entry.first));
        ui->treeWidget->addTopLevelItem(item);
    }
    delete contains_templ;
    QMessageBox::information(this, "entries", "end search");
    ui->treeWidget->setFocus();
    ui->actionIndexing->setVisible(true);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, &main_window::search_text);
}

void main_window::search_text()
{
    ui->treeWidget->clear();
    std::string text = ui->lineEdit->text().toStdString();
    if (!text.empty()) {
        ui->actionIndexing->setVisible(false);
        disconnect(ui->lineEdit, &QLineEdit::editingFinished, this, &main_window::search_text);

        progressDialog.reset(new QProgressDialog("Search text", "Cancel", 0, 100));
        connect(progressDialog.get(), &QProgressDialog::canceled, this, &main_window::stop_search);
        connect(scan.get(), &scanner::update_progress, progressDialog.get(), &QProgressDialog::setValue);

        emit new_text(text);
    }
}

void main_window::stop_index()
{
    disconnect(scan.get(), &scanner::update_progress, progressDialog.get(), &QProgressDialog::setValue);
    ui->actionIndexing->setVisible(true);
    scan_thread.get()->requestInterruption();
}

void main_window::stop_search()
{
    ui->actionIndexing->setVisible(true);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, &main_window::search_text);
    scan_thread.get()->requestInterruption();
}


main_window::~main_window() {
    if (scan_thread.get()) {
        scan_thread->quit();
    }
}
