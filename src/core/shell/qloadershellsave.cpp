// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellsave.h"
#include "qloadershell.h"
#include "qloadertree.h"

QLoaderShellSave::QLoaderShellSave(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderShellSave::exec(const QStringList &/*arguments*/)
{
    tree()->save();

    return {};
}

QString QLoaderShellSave::name() const
{
    return "save";
}

QStringList QLoaderShellSave::tab(const QStringList &/*arguments*/)
{
    return {};
}
