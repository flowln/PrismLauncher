// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QString>
#include <QDebug>
#include <exception>

class Exception : public std::exception
{
public:
    Exception(const QString &message) : std::exception(), m_message(message.toLatin1())
    {
        qCritical() << "Exception:" << message;
    }
    Exception(const Exception &other)
        : std::exception(), m_message(other.cause().toLatin1())
    {
    }
    virtual ~Exception() noexcept {}
    const char *what() const noexcept
    {
        return m_message.constData();
    }
    QString cause() const
    {
        return m_message;
    }

private:
    QByteArray m_message;
};
