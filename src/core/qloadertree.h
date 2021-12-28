/****************************************************************************
**
** Copyright (C) 2021 Sergey Naumov
**
** Permission to use, copy, modify, and/or distribute this
** software for any purpose with or without fee is hereby granted.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
** CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
** LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
** NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
** CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**
****************************************************************************/

#ifndef QLOADERTREE_H
#define QLOADERTREE_H

#include "qtloaderglobal.h"
#include <QObject>

class QLoaderTreePrivate;

class Q_LOADER_EXPORT QLoaderTree : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QLoaderTree)

protected:
    const QScopedPointer<QLoaderTreePrivate> d_ptr;

Q_SIGNALS:
    void settingsChanged();

public:
    enum Status
    {
        NoError,
        AccessError,
        FormatError,
        DesignError,
        PluginError,
        ObjectError
    };
    Q_ENUM(Status)

    explicit QLoaderTree(const QString &fileName, QObject *parent = nullptr);
    ~QLoaderTree();

    bool copy(const QStringList &section, const QStringList &to);
    QString error() const;
    int errorLine() const;
    QString fileName() const;
    bool isModified() const;
    bool load();
    bool move(const QStringList &section, const QStringList &to);
    virtual bool save();
    Status status() const;
};

#endif // QLOADERTREE_H
