// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderdata.h"
#include "qloaderstorage.h"

class QLoaderDataPrivate
{
public:
    QList<QLoaderStorage*> storageList{};
};

QLoaderData::QLoaderData(QLoaderSettings *settings, QObject *parent)
:   QObject(parent),
    QLoaderSettings(this, settings),
    d_ptr(new QLoaderDataPrivate)
{ }

QLoaderData::~QLoaderData()
{ }

void QLoaderData::addStorage(QLoaderStorage *storage)
{
    d_ptr->storageList.append(storage);
}
