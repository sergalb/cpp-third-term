#include "scanner.h"
#include <vector>
#include <future>
#include <iostream>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QQueue>
#include <QList>
#include "utf8_validator.hpp"
#include <functional>
using std::cout;
using std::endl;

scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path), index(){}

scanner::~scanner(){
    for (auto &i: index) {
        delete i.first;
        delete i.second;
    }
}

void scanner::scan_directory()
{
    //qRegisterMetaType<trigrams>("trigrams");
    QDirIterator dir_it(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    QVector<QFuture<void>> parallel_worker;
    while(dir_it.hasNext()) {
        dir_it.next();
        QFileInfo file_info = dir_it.fileInfo();
        if (!file_info.isDir()) {
            QFile * file = new QFile(file_info.absoluteFilePath());
            if (file->size() < SMALL_FILE_SIZE) {
                check_small_file(file);
            } else {
                index.push_back(qMakePair(file, new std::set<QVector<char>>()));
                parallel_worker.push_back(QtConcurrent::run(this, &scanner::validate_file, file, index.back().second));
            }
        }
    }
    for (auto i : parallel_worker) {
        i.waitForFinished();
    }

    QFuture<void> filter = QtConcurrent::filter(index, [](QPair<QFile*, trigrams> element) -> bool {
                                                            bool ans = !element.second->empty();
                                                            if (!ans) {
                                                                    delete element.first;
                                                                    delete element.second;
                                                            }
                                                            return ans;});
    filter.waitForFinished();
    emit finished();
    find_in_all_files("KMSAuto");
}

void scanner::stop()
{

}

void scanner::find_in_all_files(std::string const& templ) const
{
    QVector<QPair<int, QFile*>> &contains_templ = *new QVector<QPair<int, QFile*>>();
    if (templ.size() < SMALL_FILE_SIZE) {
        find_in_small(contains_templ, templ);
    }
    std::set<QVector<char>> templ_trigrams;
    QQueue<char> trigram;
    for (auto const &symb : templ) {
        trigram.enqueue(symb);
        if(trigram.size() == 3) {
            templ_trigrams.insert(trigram.toVector());
            trigram.dequeue();
        }
    }
    auto find_functor = std::bind(&scanner::find_substring, this, std::placeholders::_1, templ, templ_trigrams);
    QFuture<QPair<int, QFile*>> parallel_finder = QtConcurrent::mapped(index, find_functor);
    for (auto const& res : parallel_finder.results()) {
        if (res.first != -1) {
            contains_templ.push_back(res);
        }
    }
    emit substr_finded(&contains_templ);
}

void scanner::validate_file(QFile * file , trigrams file_trigrams)
{
    if (!file->open(QIODevice::ReadOnly)) {
        //todo check exception
        return;
    }
    QQueue<char> trigram;
    std::vector<char> buffer(BUFFER_SIZE);
    std::uint32_t state = UTF8_ACCEPT;
    while (!file->atEnd()) {
        size_t count_read = static_cast<size_t>(file->read(&buffer[0], BUFFER_SIZE));
        buffer.resize(count_read);
        if (validate_utf8(&state, &buffer[0], buffer.size()) == UTF8_REJECT) {
            file_trigrams->clear();
            file->close();
            return;
        }
        for (size_t i= 0; i < buffer.size(); ++i) {
            trigram.enqueue(buffer[i]);
            if (trigram.size() == 3) {
                file_trigrams->insert(trigram.toVector());
                //todo спросить про адекватность этого (костыля?): не хочу из разных тредов трогать одну память,
                //поэтому из основного треда создаю элемент в векторе и отдаю на изменение рабочему треду,
                //но по итогу работы треда я могу захотеть удалить элемент - помечу как ненужный, а потом удалю все лишнее вместе
                if (file_trigrams->size() > MAX_TRIGRAM_COUNT) {
                    file_trigrams->clear();
                    file->close();
                    return;
                }
                trigram.dequeue();
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
    std::uint32_t state = UTF8_ACCEPT;
    if (validate_utf8(&state, data, data.size()) != UTF8_REJECT) {
        small_files.push_back(qMakePair(file, data.toStdString()));
    }
    file->close();
}

QPair<int, QFile *> scanner::find_substring(QPair<QFile *, trigrams> file_tgm, std::string  const& templ, std::set<QVector<char>> const& templ_trigrams) const
{

    for (auto const& trigram : templ_trigrams) {
        if (file_tgm.second->count(trigram) == 0) {
            return qMakePair(-1, file_tgm.first);
        }
    }
    if (!file_tgm.first->open(QIODevice::ReadOnly)) {
        //TODO EXCEPTIONS
        return qMakePair(-1, file_tgm.first);
    }
    std::string buffer(BUFFER_SIZE, '0');
    size_t templ_len = 0;
    int ind_find = 0;
    while (!file_tgm.first->atEnd()) {
        size_t count_read = static_cast<size_t>(file_tgm.first->read(&buffer[templ_len], BUFFER_SIZE - templ_len));
        templ_len = templ.size();
        buffer.resize(count_read);
        auto it = std::search(buffer.begin(), buffer.end(),
                      std::boyer_moore_horspool_searcher(
                          templ.begin(), templ.end()));
        if (it != buffer.end()) {
            file_tgm.first->close();
            return qMakePair(ind_find + it - buffer.begin(), file_tgm.first);
        }
        ind_find += count_read;
        buffer.copy(&buffer[0], templ_len, count_read - templ_len);
    }
    file_tgm.first->close();
    return qMakePair(-1, file_tgm.first);
}

void scanner::find_in_small(QVector<QPair<int, QFile *>> &contains_templ, const std::string &templ) const
{
    for (auto i : small_files) {
        std::string &text = i.second;
        if (templ.size() < text.size()){
            auto it = std::search(text.begin(), text.end(),
                          std::boyer_moore_horspool_searcher(
                              templ.begin(), templ.end()));
            if (it != text.end()) {
                contains_templ.push_back(qMakePair(it - text.begin(), i.first));
            }
        }
    }
}
