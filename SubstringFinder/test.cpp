#include <iostream>
#include "scanner.h"
#include <random>
#include <QFile>
#include<algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>
#include <utility>
#include <sstream>
#include <fstream>

class Test : public QObject{
    Q_OBJECT
public:
    Test(std::string cur_dir) : cur_dir("test/" + cur_dir), scan(QString(&(this->cur_dir)[0])){
        connect(&scan, &scanner::substr_finded, this, &Test::take_ans);
    }
    std::string generate_correct(size_t size, std::string name) {
            std::ofstream in_test(name );
            char* in = new char[size];
            for (size_t i = 0; i < size; ++i) {
                in[i] = 33 + rand() % (127 - 32);
            }
            in_test.write(in, size);
            in_test.close();
            delete[] in;
            return std::string(&in[0], size/2);
    }

    void generate_incorrect(size_t size, std::string name) {
        std::ofstream in_test(name );
        char* in = new char[size];
        size += 1;
        for (size_t i = 0; i < size - 1; ++i) {
            in[i] = 33 + rand() % (127 - 32);
        }
        in[size -1] = '\0';
        in_test.write(in, size);
        in_test.close();
        delete[] in;

    }
    void check( std::vector<std::string> const& names, std::vector<std::string>const & text, std::vector<int> const& ans)
    {

        for (size_t i = 0; i < names.size(); ++i) {

        }
    }
public slots:
    void take_ans(QVector<QPair<int, QString const>> * containes_files) {
        for (int i = 0; i < containes_files->size(); ++i){
            auto ans = (*containes_files)[i];
            if (i >= names.size()) {
                std::cout << "validate fail" << std::endl;
                return;
            }
            if (ans.second.toStdString() == names[i]) {
                if (ans.first == right_ans[i]) {
                    std::cout << "pass test " << i+1 << std::endl;
                } else {
                    std::cout << "fail test " << i+1 << std::endl;
                }
            }
        }
    }
public:
    std::string cur_dir;
    scanner scan;
    std::vector<std::string> names;
    std::vector<std::string> text;
    std::vector<int> right_ans;
};


int main() {
    std::string cur_dir;
    std::cout << "enter path to cur directory" << std::endl;
    std::cin >> cur_dir;
    Test t(cur_dir);
    std::vector<std::string> names{"small_file.txt", "large_file.txt", "small_incorect.txt", "large_incorrect.txt"};
    std::vector<std::string> text;
    text.push_back(t.generate_correct(5, names[0]));
    text.push_back(t.generate_correct(BUFFER_SIZE * 4, names[1]));
    t.generate_incorrect(10, names[2]);
    t.generate_incorrect(BUFFER_SIZE * 4, names[3]);
    std::vector<int> ans{5/2, BUFFER_SIZE * 4 /2};
    t.names = names;
    t.text = text;
    t.right_ans = ans;
    t.scan.scan_directory();
    t.scan.find_in_all_files(text[0]);
    t.scan.find_in_all_files(text[1]);
}
