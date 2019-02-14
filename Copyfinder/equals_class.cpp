#include "equals_class.h"

equals_class::equals_class(){}
equals_class::equals_class(QFile *file) {
    files.push_back(qMakePair(xxh::hash64_t(), file));
}

equals_class::~equals_class()
{
    for (auto & i : files) {
        delete i.second;
    }
}

void equals_class::add_file(QFile *new_file)
{
    files.push_back(qMakePair(xxh::hash64_t(), new_file));
}

std::vector<QPair<xxh::hash64_t, QFile*>> & equals_class::get_files()
{
    return files;
}
