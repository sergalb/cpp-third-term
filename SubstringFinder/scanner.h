#ifndef SCANNER_H
#define SCANNER_H
#include <QDirIterator>
#include <QObject>
#include <QVector>
#include <QFile>
#include <QMap>
#include <memory>
#include <unordered_set>
#include <set>

typedef std::set<QVector<char>>* trigrams;
size_t const BUFFER_SIZE = 1024 * 1024;
size_t const MAX_TRIGRAM_COUNT = 40000;
size_t const SMALL_FILE_SIZE = 100;
class scanner : public QObject
{
    Q_OBJECT
public:
    explicit scanner(QObject *parent = nullptr);
    explicit scanner(QString const& root_path);
    ~scanner();
signals:
    void finished() const;
    void stoped() const;
    void substr_finded(QVector<QPair<int, QFile*>> * containes_files) const;

public slots:
    void scan_directory();
    void stop();
    void find_in_all_files(std::string const& templ) const;

private:
    void validate_file(QFile * file, trigrams file_trigrams);
    void check_small_file(QFile * file);
    QPair<int, QFile*> find_substring(QPair<QFile *, trigrams> file_tgm, std::string  const& templ, std::set<QVector<char>> const& templ_trigrams) const;
    void find_in_small(QVector<QPair<int, QFile*>> & contains_templ, std::string  const& templ) const ;

private:
    QString root_path;
    QVector<QPair<QFile*, trigrams>> index;
    QVector<QPair<QFile*, std::string>> small_files;
};
#endif // SCANNER_H
