#ifndef TRIGRAM_H
#define TRIGRAM_H
#include <QFile>
#include <unordered_set>

class trigram
{
protected:
    trigram();

private:
    QFile* file;
    std::unordered_set<std::string> *tgm;
};

#endif // TRIGRAM_H
