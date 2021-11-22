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

#include "qtloader_library.h"
#include <QObject>

struct QLoaderTreePrivate;
class Q_LOADER_EXPORT QLoaderTree : public QObject
{
    Q_OBJECT

    QScopedPointer<QLoaderTreePrivate> d_ptr;

public:
    enum Status
    {
        NoError,
        AccessError,
        FormatError,
        PluginError,
        ObjectError
    };
    Q_ENUM(Status)

    QLoaderTree(const QString &fileName, QObject *parent = nullptr);
    ~QLoaderTree();

    int fileLineNumber() const;
    QString fileName() const;
    bool load() const;
    Status status() const;
};

#endif // QLOADERTREE_H
