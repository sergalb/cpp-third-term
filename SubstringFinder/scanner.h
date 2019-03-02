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
#include <QFileSystemWatcher>
#include <QHash>
#include <utility>
#include <mutex>
typedef std::unordered_set<std::string>* trigrams;
size_t const BUFFER_SIZE = 1024 * 1024;
size_t const MAX_TRIGRAM_COUNT = 20000;
size_t const SMALL_FILE_SIZE = 100;

class scanner : public QObject
{
    Q_OBJECT
public:
    explicit scanner(QObject *parent = nullptr);
    explicit scanner(QString const& root_path);
    ~scanner();
signals:
    void substr_finded(QVector<QPair<int, QString const>> * containes_files) const;
    void update_progress(int progress) const;
    void finish_index() const;

public slots:
    void scan_directory();
    void find_in_all_files(std::string const templ) const;
private slots:
    void check_change(QString const& path);

private:
    std::pair<QString, trigrams> validate_file(QString  file, bool need_validate);
    void check_small_file(QString && file, bool need_validate);
    QPair<int, QString> find_substring(QString const& path, trigrams tgm, std::string  const& templ, std::unordered_set<std::string> const& templ_trigrams) const;
    void find_in_small(QVector<QPair<int, QString const>> & contains_templ, std::string  const& templ) const ;



private:
    mutable std::mutex search_mutex;
    QString root_path;
    QHash<QString, trigrams> index;
    QHash<QString, std::string> small_files;
    QFileSystemWatcher file_watcher;
};
#endif // SCANNER_H
