// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadersave.h"
#include "qloadershell.h"
#include "qloadertree.h"

QLoaderSave::QLoaderSave(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderSave::exec(const QStringList &/*arguments*/)
{
    tree()->save();

    return {};
}

QString QLoaderSave::name() const
{
    return "save";
}

QStringList QLoaderSave::tab(const QStringList &/*arguments*/)
{
    return {};
}
