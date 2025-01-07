// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellexit.h"
#include "qloadershell.h"

using namespace Qt::Literals::StringLiterals;

QLoaderShellExit::QLoaderShellExit(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(this, settings),
    shell(parent)
{
    parent->addCommand(this);
}

QLoaderError QLoaderShellExit::exec(const QStringList &/*arguments*/)
{
    delete shell;

    return {};
}

QString QLoaderShellExit::name() const
{
    return u"exit"_s;
}

QStringList QLoaderShellExit::tab(const QStringList &/*arguments*/)
{
    return {};
}
