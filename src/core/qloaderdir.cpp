// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderdir.h"
#include "qloaderdata.h"

QLoaderDir::QLoaderDir(QLoaderSettings *settings, QLoaderData *parent)
:   QObject(parent),
    QLoaderSettings(settings)
{ }

QLoaderDir::QLoaderDir(QLoaderSettings *settings, QLoaderDir *parent)
:   QObject(parent),
    QLoaderSettings(settings)
{ }

QIcon QLoaderDir::icon() const
{
    return {};
}
