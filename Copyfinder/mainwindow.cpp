#include "mainwindow.h"
#include "ui_mainwindow.h"
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

    scan_directory(dir);
}

void main_window::scan_directory(QString const& dir)
{
    ui->treeWidget->clear();
    setWindowTitle(QString("Directory Content - %1").arg(dir));
    split_by_size(equals_classes, QDirIterator(dir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks));
    for (auto & i : equals_classes) {
        auto split = std::async(std::launch::async, &main_window::split_by_hash, this, std::ref(i));
        split.get();
    }

}

void main_window::split_by_size(QMap<qint64, equals_class> &equals_classes, QDirIterator && dir_it)
{
    while(true) {
        QFileInfo file_info = dir_it.fileInfo();
        if (!file_info.isDir()) {
            auto map_it = equals_classes.find(file_info.size());
            if (map_it == equals_classes.end()) {
                equals_classes.insert(file_info.size(), *new equals_class(file_info.size(), new QFile(file_info.absoluteFilePath())));
            } else {
                equals_classes[file_info.size()].add_file(new QFile(file_info.absoluteFilePath()));
            }
        }
        if(dir_it.hasNext()) {
            dir_it.next();
        } else {
            break;
        }
    }

}


void main_window::split_by_hash(equals_class & cur_class)
{
    auto & files = cur_class.get_files();
    if (files.size() == 1) {
        return;
    }
    std::vector<std::future<void>> vec_fut;
    for (auto & i : files) {
        vec_fut.emplace_back(std::async(std::launch::async, [](QPair<xxh::hash64_t, QFile*> & element)  {
                QFile * file = element.second;
                xxh::hash_state64_t hash;
                if (!file->open(QIODevice::ReadOnly)) {
                    //todo обработка ошибки
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
    for (int i = 0; i < files.size() - 1; ++i) {
        if (files[i].first == files[i+1].first) {
            int count = 0;
            //QVector<QFile const&> duplicates;
            while(i + count + 1 < files.size() && files[i + count].first == files[i + count +1].first) {
                ++count;
            }
            ++count;
            for (int j = i; j < i + count; ++j) {
                QFileInfo file_info(*files[j].second);
                QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
                item->setText(0, file_info.fileName());
                item->setText(1, QString::number(count));
                item->setText(2, QString::number(file_info.size()));
                ui->treeWidget->addTopLevelItem(item);
            }
            i += count;
        }
    }

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
