#ifndef SCANNER_H
#define SCANNER_H

#include "equals_class.h"

#include <QDirIterator>
#include <QObject>
#include <QVector>
#include <QFile>
#include <QMap>

class scanner : public QObject
{
    Q_OBJECT
public:
    explicit scanner(QObject *parent = nullptr);
    explicit scanner(QString const& root_path);
signals:
    void finished();
    void return_part_duplicates(QVector<QVector<QFile*>> * duplicates);

public slots:
    void scan_directory();

private:
    QString root_path;
    QMap<qint64, equals_class*> equals_classes;
    QVector<QFile*> exceptions_files;
private:
    void split_by_size(QMap<qint64, equals_class*> & equals_classes, QDirIterator && dir_it);
    QVector<QVector<QFile*>> & split_by_hash(std::vector<QPair<xxh::hash64_t, QFile*>> &files);
};

bool check(QFile * first, QFile * second);
#endif // SCANNER_H
