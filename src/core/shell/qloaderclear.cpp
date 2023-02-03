// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderclear.h"
#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include <QPlainTextEdit>

QLoaderClear::QLoaderClear(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderClear::exec(const QStringList &/*arguments*/)
{
    if (shell->terminal())
        shell->terminal()->out()->clear();

    return {};
}

QString QLoaderClear::name() const
{
    return "clear";
}

QStringList QLoaderClear::tab(const QStringList &/*arguments*/)
{
    return {};
}
