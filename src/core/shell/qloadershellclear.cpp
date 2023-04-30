// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellclear.h"
#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include <QPlainTextEdit>

using namespace Qt::Literals::StringLiterals;

QLoaderShellClear::QLoaderShellClear(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(this, settings),
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
    return u"clear"_s;
}

QStringList QLoaderShellClear::tab(const QStringList &/*arguments*/)
{
    return {};
}
