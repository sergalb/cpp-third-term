#include "scanner.h"

scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path){}

void scanner::scan_directory()
{
    assert(root_path != nullptr && !root_path.isEmpty());
    duplicates = new QVector<QFile*>();
    split_by_size(equals_classes, QDirIterator(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks));
    for (auto & i : equals_classes) {
        std::vector<QPair<xxh::hash64_t, QFile*>> &get_files = i->get_files();
        if (get_files.size() > 1) {
            split_by_hash(get_files);
        }

    }
    emit return_duplicates(duplicates);
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


void scanner::split_by_hash(std::vector<QPair<xxh::hash64_t, QFile *>> & files)
{
    assert(files.size() > 1);
    for (auto & element : files) {
        QFile * file = element.second;
        xxh::hash_state64_t hash;
        if (!file->open(QIODevice::ReadOnly)) {
            exceptions_files.push_back(file);
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
    std::sort(files.begin(), files.end());
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
}


bool check(QFile *first, QFile *second, QVector<QFile*> & exception_files)
{
    assert(first->size() == second->size());
    std::vector<char> first_buf(BUFFER_SIZE);
    std::vector<char> second_buf(BUFFER_SIZE);
    if(!first->open(QIODevice::ReadOnly)) {
        exception_files.push_back(first);
    }
    if (!second->open(QIODevice::ReadOnly)) {
            exception_files.push_back(second);
    };
    while(!first->atEnd()) {
        //todo may be .read() throw exceptions?
        first->read(&first_buf[0], BUFFER_SIZE);
        second->read(&second_buf[0], BUFFER_SIZE);
        if (first_buf != second_buf) {
            return false;
        }
    }
    return true;
}



