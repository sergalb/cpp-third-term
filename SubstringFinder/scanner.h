#ifndef SCANNER_H
#define SCANNER_H
#include <QDirIterator>
#include <QObject>
#include <QVector>
#include <QFile>
#include <QMap>
#include <memory>
#include <unordered_set>

typedef std::unique_ptr<std::unordered_set<char[3]>> trigrams;
size_t const BUFFER_SIZE = 1024 * 1024;
size_t const MAX_TRIGRAM_COUNT = 40'000;
class scanner : public QObject
{
    Q_OBJECT
public:
    explicit scanner(QObject *parent = nullptr);
    explicit scanner(QString const& root_path);
    ~scanner();
signals:
    void finished();
    void stoped();

public slots:
    void scan_directory();
    void stop();

private:
    void validate_file(QPair<QFile *, trigrams> & file_trigrams);
    bool validate_utf8(char symbol) const noexcept;
    void check_small_file(QFile * file);

private:
    QString root_path;
    QVector<QPair<QFile*, trigrams>> index;
    QVector<QFile*> small_files;
};
#endif // SCANNER_H
