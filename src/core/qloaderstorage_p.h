// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSTORAGE_P_H
#define QLOADERSTORAGE_P_H

#include "qloadertree.h"

class QLoaderTreePrivate;
class QLoaderSettings;
class QDataStream;
class QFile;
class QLoaderBlob;

class QLoaderStoragePrivate
{
    QLoaderTreePrivate *const d_ptr;
    QFile *ofile;

    void saveRecursive(QLoaderSettings *settings, QDataStream &out);

public:
    QLoaderStoragePrivate(QLoaderTreePrivate &d);
    QLoaderBlob blob(const QUuid &uuid) const;
    QLoaderError save(QFile *ofile, QLoaderSettings *root);
    QUuid createUuid() const;
};

#endif // QLOADERSTORAGE_P_H
