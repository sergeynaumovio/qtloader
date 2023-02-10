// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellcd.h"
#include "qloadershell.h"
#include <QCommandLineParser>

class QLoaderShellCdPrivate
{
public:
    QLoaderShell *const shell;
    QCommandLineParser parser;

    QLoaderShellCdPrivate(QLoaderShell *sh)
    :   shell(sh)
    {
        parser.addPositionalArgument("object", "Change current object.");
    }
};

QLoaderShellCd::QLoaderShellCd(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    d_ptr(new QLoaderShellCdPrivate(parent))
{
    parent->addCommand(this);
}

QLoaderShellCd::~QLoaderShellCd()
{ }

QLoaderError QLoaderShellCd::exec(const QStringList &arguments)
{
    d_ptr->parser.process(arguments);
    QStringList posargs = d_ptr->parser.positionalArguments();

    if (posargs.isEmpty())
    {
        d_ptr->shell->cdHome();
        return {};
    }
    else if (posargs.size() == 1)
    {
        if (posargs.first() == ".")
            return {};

        if (posargs.first() == "..")
        {
            d_ptr->shell->cdUp();
            return {};
        }

        if (d_ptr->shell->cd(posargs.first()))
            return {};

        return {.status = QLoaderError::Object, .message = posargs.first() + ": no such object"};
    }

    return {.status = QLoaderError::Object, .message = "too many arguments"};
}

QString QLoaderShellCd::name() const
{
    return "cd";
}

QStringList QLoaderShellCd::tab(const QStringList &/*arguments*/)
{
    return {};
}
