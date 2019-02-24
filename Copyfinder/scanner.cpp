#include "scanner.h"
#include <vector>
#include <future>
#include "openfileexception.h"
#include <iostream>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path){}

scanner::~scanner()
{
    qDeleteAll(equals_classes);
}

void scanner::scan_directory()
{
    assert(root_path != nullptr && !root_path.isEmpty());
    split_by_size(equals_classes, QDirIterator(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories ));

    //вектор future от дубликатов - групп указателей на файлы
    QVector<QFuture<QVector<QVector<QFile*>>*>> split;
    for (auto & i : equals_classes) {
        if (is_stoped) {
            emit stoped();
            return;
        }
        std::vector<QPair<xxh::hash64_t, QFile*>> &get_files = i->get_files();
        if (get_files.size() <= 1) {
            continue;
        }
        QFuture<QVector<QVector<QFile*>>*> fut = QtConcurrent::run(this, &scanner::split_by_hash, std::ref(get_files));
        split.push_back(fut);
    }
    for (auto & i : split) {
        if (is_stoped) {
            emit stoped();
            return;
        }
        QVector<QVector<QFile*>> * part_duplicates = i.result();
        if (!part_duplicates->isEmpty()) {
            emit return_part_duplicates(part_duplicates);
        } else {
            delete part_duplicates;
        }
    }
    emit finished();
}

void scanner::stop()
{
    is_stoped = true;
}

void scanner::split_by_size(QMap<qint64, equals_class*> &equals_classes, QDirIterator && dir_it)
{
    while(dir_it.hasNext()) {
        if (is_stoped) {
            return;
        }
        dir_it.next();
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

    }
}

QVector<QVector<QFile*>> * scanner::split_by_hash(std::vector<QPair<xxh::hash64_t, QFile *>> & files)
{
    assert(files.size() > 1);
    calc_hash(files);
    std::sort(files.begin(), files.end(), [](QPair<xxh::hash64_t, QFile *> const & first, QPair<xxh::hash64_t, QFile *> const & second) {return first.first < second.first;});
    QVector<QVector<QFile*>> & duplicates = *new QVector<QVector<QFile*>>();
    for (size_t i = 0; i < files.size() - 1; ++i) {
        if (is_stoped) {
            return &duplicates;
        }
        if (files[i].first == files[i+1].first) {
            size_t count = 1;
            while(i + count < files.size() && files[i + count - 1].first == files[i + count].first) {
                ++count;
            }
            duplicates.push_back(QVector<QFile*>(1, files[i].second));
            for (size_t j = i + 1; j < i + count; ++j) {
                bool added = false;
                bool exception = false;
                for (auto & group  : duplicates) {
                    try {                       
                        if (check(group[0], files[j].second)) {
                            added = true;
                            group.push_back(files[j].second);
                            break;
                        }
                    } catch(OpenFileException &e) {
                        //std::cout << e.what() << std::endl;
                        exception = true;
                        continue;
                    }
                }
                if (exception) {
                    continue;
                }
                if (!added) {
                    duplicates.push_back(QVector<QFile*>(1, files[j].second));
                }
            }
            i += count;
        }
    }
    return &duplicates;
}

void calc_hash(std::vector<QPair<xxh::hash64_t, QFile *>> & files) {
    for (auto & element : files) {
        QFile * file = element.second;
        xxh::hash_state64_t hash;
        if (!file->open(QIODevice::ReadOnly)) {
            //exceptions_files.push_back(file);
            continue;
        }
        std::vector<char> buffer(BUFFER_SIZE);
        while (!file->atEnd()) {
           size_t count_read = static_cast<size_t>(file->read(&buffer[0], BUFFER_SIZE));
            buffer.resize(count_read);
            hash.update(buffer);
        }
        file->close();
        element.first = hash.digest();
    }
}

bool check(QFile *first, QFile *second)
{
    std::vector<char> first_buf(BUFFER_SIZE);
    std::vector<char> second_buf(BUFFER_SIZE);   
    if(!first->open(QIODevice::ReadOnly)) {
        throw *new OpenFileException();
    }
    if (!second->open(QIODevice::ReadOnly)) {
        first->close();
        throw OpenFileException();
    };
    while(!first->atEnd()) {
        //todo may be .read() throw exceptions?
        first->read(&first_buf[0], BUFFER_SIZE);
        second->read(&second_buf[0], BUFFER_SIZE);
        if (first_buf != second_buf) {
            first->close();
            second->close();
            return false;
        }
    }
    first->close();
    second->close();
    return true;
}
