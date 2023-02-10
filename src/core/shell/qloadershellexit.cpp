// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellexit.h"
#include "qloadershell.h"

QLoaderShellExit::QLoaderShellExit(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderShellExit::exec(const QStringList &/*arguments*/)
{
    shell->deleteLater();

    return {};
}

QString QLoaderShellExit::name() const
{
    return "exit";
}

QStringList QLoaderShellExit::tab(const QStringList &/*arguments*/)
{
    return {};
}
