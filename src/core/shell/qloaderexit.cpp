// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderexit.h"
#include "qloadershell.h"

QLoaderExit::QLoaderExit(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderExit::exec(const QStringList &/*arguments*/)
{
    shell->deleteLater();

    return {};
}

QString QLoaderExit::name() const
{
    return "exit";
}

QStringList QLoaderExit::tab(const QStringList &/*arguments*/)
{
    return {};
}
