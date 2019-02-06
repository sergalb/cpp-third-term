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
    connect(ui->actionFind_copy, &QAction::triggered, this, &main_window::select_directory);
}

void main_window::select_directory()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for find copy",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    qDeleteAll(equals_classes.begin(), equals_classes.end());
    equals_classes.clear();
    scan_directory(dir);
}

void main_window::scan_directory(QString const& dir)
{
    qRegisterMetaType<QVector<int>>("QVector<int>");
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    split_by_size(equals_classes, QDirIterator(dir, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks));
    auto duplicates = equals_classes.operator[](0)->get_files();
    for (size_t i = 1; i < 100000; ++i) {
        if (equals_classes.find(i)!= equals_classes.end()) {
            auto get = equals_classes.operator[](i)->get_files();
            for (auto j : get) {
                duplicates.push_back(j);
            }
        }
    }
    std::vector<std::future<std::vector<QFile*> &>> split;
    for (auto & i : equals_classes) {
        std::vector<QPair<xxh::hash64_t, QFile*>> &get_files = i->get_files();
        if (get_files.size() <= 1) {
            continue;
        }
        split.emplace_back(std::async(std::launch::async, &main_window::split_by_hash, this, std::ref(get_files)));
    }
    int spliter = 0;
    for (auto & i : split) {
        std::vector<QFile *> duplicates = i.get();
        for (size_t j = 0; j < duplicates.size(); ++j) {
            QFileInfo file_info(*duplicates[j]);
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
            std::cout << file_info.fileName().toStdString() << std::endl;
            item->setText(0, file_info.fileName());
            item->setText(1, QString::number(duplicates.size()));
            item->setText(2, QString::number(file_info.size()));
            ui->treeWidget->addTopLevelItem(item);
            //delete item;
        }
        if (spliter == 15) {
            QCoreApplication::processEvents();
            spliter = 0;
        }
        ++spliter;
    }
    //todo это должно происходить не здесь, указатели на дубликаты хочу оставить, хотя все остальное неплохо бы выкинуть
    qDeleteAll(equals_classes.begin(), equals_classes.end());
    equals_classes.clear();
}

void main_window::split_by_size(QMap<qint64, equals_class*> &equals_classes, QDirIterator && dir_it)
{
    while(true) {
        QFileInfo file_info = dir_it.fileInfo();
        if (!file_info.isDir()) {
            qint64 file_size = file_info.size();
            auto map_it = equals_classes.find(file_size);
            if (map_it == equals_classes.end()) {
                equals_classes.insert(file_size, new equals_class(new QFile(file_info.absoluteFilePath())));
            } else {
                equals_classes[file_size]->add_file(new QFile(file_info.absoluteFilePath()));
            }
        }
        if (dir_it.hasNext()) {
            dir_it.next();
        } else {
            break;
        }
    }

}


std::vector<QFile*> & main_window::split_by_hash(std::vector<QPair<xxh::hash64_t, QFile *>> & files)
{
    assert(files.size() > 1);
    std::vector<std::future<void>> vec_fut;
    for (auto & i : files) {
        vec_fut.emplace_back(std::async(std::launch::async, [](QPair<xxh::hash64_t, QFile*> & element)  {
                QFile * file = element.second;
                xxh::hash_state64_t hash;
                if (!file->open(QIODevice::ReadOnly)) {
                    //todo обработка ошибки
                     return;
                }
                std::vector<char> buffer(BUFFER_SIZE);
                while (!file->atEnd()) {
                    size_t count_read = static_cast<size_t>(file->read(&buffer[0], BUFFER_SIZE));
                    buffer.resize(count_read);
                    hash.update(buffer);
                }
                file->close();
                element.first = hash.digest();
        }, std::ref(i)));
    }
    for(auto & i: vec_fut) {
        i.wait();
    }
    std::sort(files.begin(), files.end());
    std::vector<QFile *> *duplicates = new std::vector<QFile *>;
    for (size_t i = 0; i < files.size() - 1; ++i) {
        if (files[i].first == files[i+1].first) {
            size_t count = 0;
            //можно бинарник впихнуть, но зачем
            while(i + count + 1 < files.size() && files[i + count].first == files[i + count +1].first) {
                duplicates->push_back(files[i+count++].second);
            }
            duplicates->push_back(files[i+count++].second);
            i += count;
        }
    }
    return *duplicates;
}


main_window::~main_window()
{}

bool check(QFile *first, QFile *second)
{
    assert(first->size() == second->size());
    std::vector<char> first_buf(BUFFER_SIZE);
    std::vector<char> second_buf(BUFFER_SIZE);
    if(!first->open(QIODevice::ReadOnly)) {
        //throw QException("can't open file" + first.fileName());
    }
    second->open(QIODevice::ReadOnly);
    while(!first->atEnd()) {

        first->read(&first_buf[0], BUFFER_SIZE);
        second->read(&second_buf[0], BUFFER_SIZE);
        if (first_buf != second_buf) {
            return false;
        }

    }
    return true;
}
