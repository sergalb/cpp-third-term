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
#include <QHash>
#include <utility>
#include <mutex>
#include <algorithm>
using std::cout;
using std::endl;

scanner::scanner(QObject *parent) : QObject(parent){}

scanner::scanner(const QString &root_path) : root_path(root_path), index(){
    connect(&file_watcher, &QFileSystemWatcher::fileChanged, this, &scanner::check_change);
}

scanner::~scanner(){
    for (auto i = index.begin(); i != index.end(); ++i) {
        delete i.value();
    }
}

void scanner::scan_directory()
{
    //qRegisterMetaType<trigrams>("trigrams");
    QDirIterator dir_it(root_path, QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
    QVector<QFuture<std::pair<QString, trigrams>>> parallel_worker;
    while(dir_it.hasNext()) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            for (auto i : parallel_worker) {
                auto res = i.result();
                delete res.second;
            }
            break;
        }
        dir_it.next();
        QFileInfo file_info = dir_it.fileInfo();
        if (!file_info.isDir()) {
            if (static_cast<size_t>(file_info.size()) < SMALL_FILE_SIZE) {
                check_small_file(QString(file_info.absoluteFilePath()), true);
            } else {
                parallel_worker.push_back(QtConcurrent::run(this, &scanner::validate_file, QString(file_info.absoluteFilePath()), true));
            }
        }
    }
    if (QThread::currentThread()->isInterruptionRequested()) {
        return;
    }
    emit update_progress(15);
    int step = std::max(1, parallel_worker.size()/84);
    int progress = 0;
    for (auto i : parallel_worker) {
        auto res = i.result();
        if (QThread::currentThread()->isInterruptionRequested()) {
            delete res.second;
            continue;
        }
        if (++progress % step == 0) {
            emit update_progress(15 + progress/step);
        }
        if (res.second != nullptr) {
            index.insert(res.first, res.second);
        }
    }
    if (QThread::currentThread()->isInterruptionRequested()) {
        return;
    }
    QList<QString> keys = index.keys();
    for (auto &file_path : keys) {
        file_watcher.addPath(file_path);
    }

    connect(&file_watcher, &QFileSystemWatcher::fileChanged, this, &scanner::check_change);
    emit update_progress(100);
    emit finish_index();
}

void scanner::find_in_all_files(std::string const templ) const
{
    std::lock_guard<std::mutex> lock(search_mutex);
    QVector<QPair<int, QString const>> &contains_templ = *new QVector<QPair<int, QString const>>();    
    if (templ.size() < SMALL_FILE_SIZE) {
        find_in_small(contains_templ, templ);
    }
    emit update_progress(20);
    std::unordered_set<std::string> templ_trigrams;
    QQueue<char> trigram;
    for (auto const &symb : templ) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            delete &contains_templ;
            return;
        }
        trigram.enqueue(symb);
        if(trigram.size() == 3) {
            std::string str(3, '0');
            size_t ind = 0;
            for (auto &i : trigram) {
                str[ind++] = i;
            }
            templ_trigrams.insert(std::move(str));
            trigram.dequeue();
        }
    }
    //auto find_functor = std::bind(&scanner::find_substring, this, std::placeholders::_1, std::placeholders::_2, templ, templ_trigrams);
    int step = std::max(1, index.size()/10);
    int progress = 0;
    QVector<QFuture<QPair<int, QString >>> parallel_finder;
    for (auto i = index.begin(); i != index.end(); ++i) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            for (auto &i : parallel_finder) {
                i.result();
            }
            delete &contains_templ;
            return;
        }
        parallel_finder.push_back(QtConcurrent::run(this, &scanner::find_substring,
                                                    std::cref(i.key()), i.value(), std::cref(templ),
                                                    std::cref(templ_trigrams)));
        if (++progress % step == 0) {
            emit update_progress(20 + progress/step);
        }
    }
    step = std::max(1,index.size()/69);
    progress = 0;
    for (auto i : parallel_finder) {
        if (QThread::currentThread()->isInterruptionRequested()) {
            delete &contains_templ;
            for (auto &i : parallel_finder) {
                i.result();
            }
            return;
        }
        auto res = i.result();
        if (res.first != -1) {
            contains_templ.push_back(res);
        }
        if (++progress % step == 0) {
            emit update_progress(30 + progress/step);
        }
    }
    emit update_progress(100);
    emit substr_finded(&contains_templ);
}

void scanner::check_change(const QString &path)
{
    std::lock_guard<std::mutex> lock(search_mutex);
    if (small_files.contains(path)) {
        check_small_file(QString(path), false);
    } else if (index.contains(path)) {
        validate_file(path, false);
    }
}

std::pair<QString, trigrams> scanner::validate_file(QString path, bool need_validate)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout<< "Can't open file: " << path.toStdString() << endl;
        return std::make_pair(path, nullptr);
    }

    QQueue<char> trigram;
    std::vector<char> buffer(BUFFER_SIZE);
    std::uint32_t state = UTF8_ACCEPT;
    trigrams file_trigrams = new std::unordered_set<std::string>();
    while (!file.atEnd()) {
        qint64 count_read = (file.read(&buffer[0], BUFFER_SIZE));
        if (count_read == -1) {
            std::cout<< "Can't open file: " << path.toStdString() << endl;
            delete file_trigrams;
            file.close();
            return std::make_pair(path, nullptr);
        }
        buffer.resize(static_cast<size_t>(count_read));
        if (need_validate && validate_utf8(&state, &buffer[0], buffer.size()) == UTF8_REJECT) {
            delete file_trigrams;            
            file.close();
            return std::make_pair(path, nullptr);
        }
        for (size_t i= 0; i < buffer.size(); ++i) {
            trigram.enqueue(buffer[i]);
            if (trigram.size() == 3) {
                std::string str(3, '\0');
                size_t ind = 0;
                for (auto &i : trigram) {
                    str[ind++] = i;
                }
                file_trigrams->insert(std::move(str));
                if (need_validate && file_trigrams->size() > MAX_TRIGRAM_COUNT) {
                    delete file_trigrams;
                    file.close();
                    return std::make_pair(path, nullptr);
                }
                trigram.dequeue();
            }
        }
    }
    file.close();
    return std::make_pair(path, file_trigrams);
}

void scanner::check_small_file(QString && path, bool need_validate)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout<< "Can't open file: " << path.toStdString() << endl;
        return;
    }
    QByteArray data = file.read(file.size());
    std::uint32_t state = UTF8_ACCEPT;
    if (!need_validate || validate_utf8(&state, data, static_cast<size_t>(data.size())) != UTF8_REJECT) {
        small_files.insert(path, data.toStdString());
        file_watcher.addPath(path);
    }
    file.close();
}

QPair<int, QString> scanner::find_substring(QString const& path, trigrams tgm, std::string const& templ, std::unordered_set<std::string> const& templ_trigrams) const
{
    //for (auto it = templ_trigrams.begin(); it != templ_trigrams.end(); ++it){
    for (auto templ_tgm : templ_trigrams) {
        if (tgm->count(templ_tgm) == 0) {
            return qMakePair(-1, path);
        }
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cout<< "Can't open file: " << path.toStdString() << endl;
        return qMakePair(-1, path);
    }
    std::string buffer(BUFFER_SIZE, '0');
    size_t templ_len = 0;
    int ind_find = 0;
    while (!file.atEnd()) {
        qint64 count_read = (file.read(&buffer[templ_len], static_cast<qint64>(BUFFER_SIZE - templ_len)));
        if(count_read == -1 ) {
            cout << "Can't read file: " << path.toStdString() << endl;
            file.close();
            return qMakePair(-1, path);
        }
        templ_len = templ.size();
        buffer.resize(static_cast<size_t>(count_read));
        auto it = std::search(buffer.begin(), buffer.end(),
                      std::boyer_moore_horspool_searcher(
                          templ.begin(), templ.end()));
        if (it != buffer.end()) {
            file.close();
            return qMakePair(ind_find + it - buffer.begin(), path);
        }
        ind_find += count_read;
        buffer.copy(&buffer[0], templ_len, static_cast<size_t>(count_read) - templ_len);
    }
    file.close();
    return qMakePair(-1, path);
}

void scanner::find_in_small(QVector<QPair<int, QString const >> &contains_templ, const std::string &templ) const
{

    int step = std::max(1, small_files.size()/20);
    int ind =0;
    for (auto i = small_files.begin(); i != small_files.end(); ++i) {
        if(++ind % step == 0) {
            emit update_progress(ind/step);
        }
        std::string const& text = i.value();
        if (templ.size() < text.size()){
            auto it = std::search(text.begin(), text.end(),
                          std::boyer_moore_horspool_searcher(
                              templ.begin(), templ.end()));
            if (it != text.end()) {
                contains_templ.push_back(qMakePair(it - text.begin(), i.key()));
            }
        }
    }
}
