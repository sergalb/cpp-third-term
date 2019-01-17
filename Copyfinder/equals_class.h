#ifndef EQUALS_CLASS_H
#define EQUALS_CLASS_H

#include <map>
#include <set>
#include <QFile>
#include <QVector>
#include "xxhash_cpp/xxhash/xxhash.hpp"
size_t const BUFFER_SIZE = 1024 * 1024;
class equals_class
{
public:
    equals_class();
    equals_class(qint64 weight, QFile *file);
    void add_file(QFile *new_file);
    QVector<QPair<xxh::hash64_t, QFile*>> & get_files();

private:
    qint64 weight;
    QVector<QPair<xxh::hash64_t, QFile*>> files;
};

#endif // EQUALS_CLASS_H
