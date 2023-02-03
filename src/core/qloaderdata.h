// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERDATA_H
#define QLOADERDATA_H

#include "qloadersettings.h"

class QLoaderDataPrivate;
class QLoaderStorage;

class Q_LOADER_EXPORT QLoaderData : public QObject, public QLoaderSettings
{
    Q_OBJECT

    const QScopedPointer<QLoaderDataPrivate> d_ptr;

public:
    QLoaderData(QLoaderSettings *settings, QObject *parent);
    ~QLoaderData();

    void addStorage(QLoaderStorage *blob);
};

#endif // QLOADERDATA_H
