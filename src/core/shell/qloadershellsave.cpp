// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellsave.h"
#include "qloadershell.h"
#include "qloadertree.h"

using namespace Qt::Literals::StringLiterals;

QLoaderShellSave::QLoaderShellSave(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(this, settings),
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
    return u"save"_s;
}

QStringList QLoaderShellSave::tab(const QStringList &/*arguments*/)
{
    return {};
}
