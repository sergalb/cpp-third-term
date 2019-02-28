#include "scanner.h"
#include <vector>
#include <future>
#include <iostream>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QQueue>
#include <QList>

scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path){}

scanner::~scanner()
{

}

void scanner::scan_directory()
{
    QDirIterator dir_it(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    QVector<QFuture<void>> parallel_worker;
    while(dir_it.hasNext()) {
        dir_it.next();
        QFileInfo file_info = dir_it.fileInfo();
        if (!file_info.isDir()) {
            QFile * file = new QFile(file_info.absoluteFilePath());
            if (file->size() < 3) {
                check_small_file(file);
            } else {
                index.push_back(qMakePair(file, trigrams(new std::unordered_set<char[3]>())));
                parallel_worker.push_back(QtConcurrent::run(this, &scanner::validate_file, std::ref(index.back())));
            }
        }
    }
    for (auto i : parallel_worker) {
        i.waitForFinished();
    }
    QFuture<void> filter = QtConcurrent::filter(index, [](QPair<QFile*, trigrams> element) -> bool {
                                                            return element.second.get()!= nullptr; });
    filter.waitForFinished();
}

void scanner::stop()
{

}

void scanner::validate_file(QPair<QFile *, trigrams> &file_trigrams)
{
    QFile *file = file_trigrams.first;
    if (!file->open(QIODevice::ReadOnly)) {
        //todo check exception
        return;
    }
    QQueue<char> trigram;
    std::vector<char> buffer(BUFFER_SIZE);
    while (!file->atEnd()) {
        size_t count_read = static_cast<size_t>(file->read(&buffer[0], BUFFER_SIZE));
        buffer.resize(count_read);
        for (size_t i= 0; i < buffer.size(); ++i) {
            if (validate_utf8(buffer[i])) {
                trigram.enqueue(buffer[i]);
                if (trigram.size() == 3) {
                    char new_trigram[3];
                    size_t j = 0;
                    for (char symb : trigram) {
                        new_trigram[j++] = symb;
                    }
                    file_trigrams.second->insert(new_trigram);
                    //todo спросить про адекватность этого (костыля?): не хочу из разных тредов трогать одну память,
                    //поэтому из основного треда создаю элемент в векторе и отдаю на изменение рабочему треду,
                    //но по итогу работы треда я могу захотеть удалить элемент - помечу как ненужный, а потом удалю все лишнее вместе
                    if (file_trigrams.second->size() > MAX_TRIGRAM_COUNT) {
                        file_trigrams.second.reset();
                        return;
                    }
                    trigram.dequeue();
                }
            }
        }
    }
    file->close();
}

void scanner::check_small_file(QFile *file)
{
    if (!file->open(QIODevice::ReadOnly)) {
        //todo check exception
        return;
    }
    QByteArray data = file->read(file->size());
    for (int i = 0; i < file->size(); ++i) {
        if (!validate_utf8(data[i])) {
            return;
        }
    }
    small_files.push_back(file);
}
