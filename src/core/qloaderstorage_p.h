/****************************************************************************
**
** Copyright (C) 2022 Sergey Naumov
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

#ifndef QLOADERSTORAGE_P_H
#define QLOADERSTORAGE_P_H

#include "qloadertree.h"
#include <QHash>
#include <QUuid>

class QLoaderTreePrivate;
class QLoaderSettings;
class QDataStream;

class QLoaderStoragePrivate
{
    QLoaderTreePrivate *const d_ptr;
    QHash<QUuid, qint64> seek;

    void saveRecursive(QLoaderSettings *settings, QDataStream &out);

public:
    QLoaderStoragePrivate(QLoaderTreePrivate &d);
    QByteArray blob(const QUuid &uuid) const;
    QLoaderTree::Error save(QLoaderSettings *root);
    QUuid uuid() const;
};

#endif // QLOADERSTORAGE_P_H
