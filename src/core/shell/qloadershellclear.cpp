// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellclear.h"
#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include <QPlainTextEdit>

QLoaderShellClear::QLoaderShellClear(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderShellClear::exec(const QStringList &/*arguments*/)
{
    if (shell->terminal())
        shell->terminal()->out()->clear();

    return {};
}

QString QLoaderShellClear::name() const
{
    return "clear";
}

QStringList QLoaderShellClear::tab(const QStringList &/*arguments*/)
{
    return {};
}
