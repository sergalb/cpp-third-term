    #include "equals_class.h"

equals_class::equals_class(){}
equals_class::equals_class(qint64 weight, QFile *file) : weight(weight){
    files.push_back(qMakePair(xxh::hash64_t(), file));
}

void equals_class::add_file(QFile *new_file)
{
    files.push_back(qMakePair(xxh::hash64_t(), new_file));
}

QVector<QPair<xxh::hash64_t, QFile*>> &equals_class::get_files()
{
    return files;
}
