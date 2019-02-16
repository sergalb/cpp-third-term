#ifndef OPENFILEEXCEPTION_H
#define OPENFILEEXCEPTION_H
#include <QException>

class OpenFileException : QException
{
public:
    OpenFileException(){}
    void raise() const override { throw *this; }
    OpenFileException *clone() const override { return new OpenFileException(*this); }
};

#endif // OPENFILEEXCEPTION_H
