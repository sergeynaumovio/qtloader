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
#include "qloadersettings.h"
#include <QFile>

void QLoaderStoragePrivate::saveRecursive(QLoaderSettings *settings, QDataStream &out)
{
    const QLoaderSettingsData &item = d_ptr->hash.data[settings];
    QMapIterator<QString, QLoaderProperty> i(item.properties);
    while (i.hasNext())
    {
        i.next();

        QLoaderProperty property = i.value();
        if (property.isValue)
            continue;

        QUuid uuid(d_ptr->fromString(property).toByteArray());
        if (uuid.isNull())
            continue;

        QByteArray text = uuid.toByteArray(QUuid::WithoutBraces) + " = QLoaderBlob(";
        out.writeRawData(text.data(), text.size());

        struct
        {
            qint64 start;
            qint64 end;

        } pos;

        pos.start = ofile->pos();
        QByteArray data = item.settings->saveBlob(i.key());
        if (data.isNull())
            data = blob(uuid);

        d_ptr->hash.blobs[uuid] = pos.start;

        out << qint64(/*pos.next*/) << QByteArray(/*header*/) << data;

        text = ")\n";
        out.writeRawData(text.data(), text.size());

        pos.end = ofile->pos();
        ofile->seek(pos.start);
        qint64 next = pos.end - pos.start - sizeof(qint64);
        out << next;
        ofile->seek(pos.end);
    }

    for (QLoaderSettings *child : item.children)
        saveRecursive(child, out);
}

QLoaderStoragePrivate::QLoaderStoragePrivate(QLoaderTreePrivate &d)
:   d_ptr(&d)
{ }

QByteArray QLoaderStoragePrivate::blob(const QUuid &uuid) const
{
    if (d_ptr->hash.blobs.contains(uuid))
    {
        struct
        {
            qint64 start;
            qint64 next;

        } pos;

        pos.start = d_ptr->hash.blobs[uuid];
        d_ptr->file->seek(pos.start);
        QDataStream in(d_ptr->file);
        QByteArray header;
        QByteArray data;
        in >> pos.next >> header >> data;
        return data;
    }

    return {};
}

QLoaderTree::Error QLoaderStoragePrivate::save(QFile *outfile, QLoaderSettings *root)
{
    ofile = outfile;
    QLoaderTree::Error error{.status = QLoaderTree::AccessError, .message = "read-only file"};
    if (!ofile->open(QIODevice::ReadWrite))
        return error;

    ofile->seek(ofile->size());

    QDataStream out(ofile);
    saveRecursive(root, out);
    ofile->close();

    return {};
}

QUuid QLoaderStoragePrivate::createUuid() const
{
    d_ptr->mutex.lock();
    QUuid uuid;
    do { uuid = QUuid::createUuid(); } while (d_ptr->hash.blobs.contains(uuid));
    d_ptr->mutex.unlock();

    return uuid;
}
