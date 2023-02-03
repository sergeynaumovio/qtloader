// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderstorage_p.h"
#include "qloadertree_p.h"
#include "qloadersettings.h"
#include <QFile>

using Version = qint32;

struct Position
{
    qint64 start{};
    qint64 end{};
    qint64 next{};
};

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

        QByteArray array = uuid.toByteArray(QUuid::WithoutBraces) + " = QLoaderBlob(";
        out.writeRawData(array.data(), array.size());

        Position pos{.start = ofile->pos()};
        QLoaderBlob bo = item.settings->saveBlob(i.key());
        if (bo.array.isNull())
            bo = blob(uuid);

        d_ptr->hash.blobs[uuid] = pos.start;

        out << pos.next << static_cast<Version>(bo.version) << QByteArray(/*header*/) << bo.array;

        array = ")\n";
        out.writeRawData(array.data(), array.size());

        pos.end = ofile->pos();
        ofile->seek(pos.start);
        pos.next = pos.end - pos.start - sizeof(qint64);
        out << pos.next;
        ofile->seek(pos.end);
    }

    for (QLoaderSettings *child : item.children)
        saveRecursive(child, out);
}

QLoaderStoragePrivate::QLoaderStoragePrivate(QLoaderTreePrivate &d)
:   d_ptr(&d)
{ }

QLoaderBlob QLoaderStoragePrivate::blob(const QUuid &uuid) const
{
    if (d_ptr->hash.blobs.contains(uuid))
    {
        Position pos{.start = d_ptr->hash.blobs[uuid]};
        d_ptr->file->seek(pos.start);
        QDataStream in(d_ptr->file);
        Version version;
        in >> pos.next >> version;
        in.setVersion(static_cast<QDataStream::Version>(version));
        QByteArray header;
        QByteArray data;
        in >> header >> data;
        return {data, static_cast<QDataStream::Version>(version)};
    }

    return {};
}

QLoaderError QLoaderStoragePrivate::save(QFile *outfile, QLoaderSettings *root)
{
    ofile = outfile;
    QLoaderError error{.status = QLoaderError::Access, .message = "read-only file"};
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
