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

#include "qloaderstorage_p.h"
#include "qloadertree_p.h"
#include <QFile>

void QLoaderStoragePrivate::saveRecursive(QLoaderSettings */*settings*/, QDataStream &/*out*/)
{

}

QLoaderStoragePrivate::QLoaderStoragePrivate(QLoaderTreePrivate &d)
:   d_ptr(&d)
{ }

QByteArray QLoaderStoragePrivate::blob(const QUuid &uuid) const
{
    d_ptr->file->seek(seek[uuid]);
    return {};
}

QLoaderTree::Error QLoaderStoragePrivate::save(QLoaderSettings *root)
{
    QLoaderTree::Error error;
    if (!d_ptr->file->open(QIODevice::WriteOnly | QIODevice::Append))
    {
        error.status = QLoaderTree::AccessError;
        error.message = "read-only file";
        return error;
    }

    QDataStream out(d_ptr->file);
    saveRecursive(root, out);
    d_ptr->file->close();

    return error;
}

QUuid QLoaderStoragePrivate::uuid() const
{
    QUuid uuid = QUuid::createUuid();
    d_ptr->mutex.lock();
    while (seek.contains(uuid))
        uuid = QUuid::createUuid();
    d_ptr->mutex.unlock();

    return uuid;
}
