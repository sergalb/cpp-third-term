#ifndef EQUALS_CLASS_H
#define EQUALS_CLASS_H

#include <map>
#include <set>
#include <QFile>
#include <vector>
#include "xxhash_cpp/xxhash/xxhash.hpp"
size_t const BUFFER_SIZE = 1024 * 1024;
class equals_class
{
public:
    equals_class();
    explicit equals_class(QFile *file);
    ~equals_class();
    void add_file(QFile *new_file);
    std::vector<QPair<xxh::hash64_t, QFile*>> & get_files();

private:
    std::vector<QPair<xxh::hash64_t, QFile*>> files;
};

#endif // EQUALS_CLASS_H
