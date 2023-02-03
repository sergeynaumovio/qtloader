// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSTORAGE_H
#define QLOADERSTORAGE_H

#include "qloadersettings.h"

class QLoaderStoragePrivate;
class QLoaderTreePrivate;

class QLoaderStorage : public QObject, public QLoaderSettings
{
    Q_OBJECT

    const QScopedPointer<QLoaderStoragePrivate> d_ptr;

public:
    QLoaderStorage(QLoaderTreePrivate &d, QLoaderSettings *settings, QObject *parent);
    ~QLoaderStorage();
};

#endif // QLOADERSTORAGE_H
