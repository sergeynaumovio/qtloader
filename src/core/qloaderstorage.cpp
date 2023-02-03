// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderstorage.h"
#include "qloaderstorage_p.h"
#include "qloadertree_p.h"

QLoaderStorage::QLoaderStorage(QLoaderTreePrivate &d, QLoaderSettings *settings, QObject *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    d_ptr(new QLoaderStoragePrivate(d))
{
    d.setStorageData(*d_ptr);
}

QLoaderStorage::~QLoaderStorage()
{ }
