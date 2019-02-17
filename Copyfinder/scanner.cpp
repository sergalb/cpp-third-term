#include "scanner.h"
#include <vector>
#include <future>
#include "openfileexception.h"
#include <iostream>
scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path){}

void scanner::scan_directory()
{
    assert(root_path != nullptr && !root_path.isEmpty());
    split_by_size(equals_classes, QDirIterator(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories ));
    std::vector<std::future<QVector<QVector<QFile*>> & >> split;
    for (auto & i : equals_classes) {
        std::vector<QPair<xxh::hash64_t, QFile*>> &get_files = i->get_files();
        if (get_files.size() <= 1) {
            continue;
        }
        split.emplace_back(std::async(std::launch::async, &scanner::split_by_hash, this, std::ref(get_files)));
    }
    for (auto & i : split) {
        auto part_duplicates = new QVector<QVector<QFile*>>(i.get());
        if (!part_duplicates->isEmpty()) {
            emit return_part_duplicates(part_duplicates);
        } else {
            delete part_duplicates;
        }
    }
    emit finished();
}

void scanner::split_by_size(QMap<qint64, equals_class*> &equals_classes, QDirIterator && dir_it)
{
    while(dir_it.hasNext()) {
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

QVector<QVector<QFile*>> & scanner::split_by_hash(std::vector<QPair<xxh::hash64_t, QFile *>> & files)
{
    assert(files.size() > 1);
    //calc hash
    for (auto & element : files) {
        QFile * file = element.second;
        xxh::hash_state64_t hash;
        if (!file->open(QIODevice::ReadOnly)) {
            //exceptions_files.push_back(file);
            std::cout << "exception in calc hash" << std::endl;
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
    std::sort(files.begin(), files.end(), [](QPair<xxh::hash64_t, QFile *> const & first, QPair<xxh::hash64_t, QFile *> const & second) {return first.first < second.first;});
    QVector<QVector<QFile*>> & duplicates = *new QVector<QVector<QFile*>>();
    for (size_t i = 0; i < files.size() - 1; ++i) {
        if (files[i].first == files[i+1].first) {
            size_t count = 1;
            while(i + count < files.size() && files[i + count - 1].first == files[i + count].first) {
                ++count;
            }
            duplicates.push_back(*new QVector<QFile*>(1, files[i].second));

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
                        //std::cout << "exception" << std::endl;
                        exception = true;
                        continue;
                    }
                }
                if (exception) {
                    continue;
                }
                if (!added) {
                    duplicates.push_back(*new QVector<QFile*>(1, files[j].second));
                }
            }
            i += count;
        }
    }
    return duplicates;
}


bool check(QFile *first, QFile *second)
{
    std::vector<char> first_buf(BUFFER_SIZE);
    std::vector<char> second_buf(BUFFER_SIZE);   
    if(!first->open(QIODevice::ReadOnly)) {
        std::cout << "can't open first" << std::endl;
        throw *new OpenFileException();
    }
    if (!second->open(QIODevice::ReadOnly)) {
        first->close();
        std::cout << "can't open second" << std::endl;
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
