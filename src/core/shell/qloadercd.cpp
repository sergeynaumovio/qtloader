// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadercd.h"
#include "qloadershell.h"
#include <QCommandLineParser>

class QLoaderCdPrivate
{
public:
    QLoaderShell *const shell;
    QCommandLineParser parser;

    QLoaderCdPrivate(QLoaderShell *sh)
    :   shell(sh)
    {
        parser.addPositionalArgument("object", "Change current object.");
    }
};

QLoaderCd::QLoaderCd(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    d_ptr(new QLoaderCdPrivate(parent))
{
    parent->addCommand(this);
}

QLoaderCd::~QLoaderCd()
{ }

QLoaderError QLoaderCd::exec(const QStringList &arguments)
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

QString QLoaderCd::name() const
{
    return "cd";
}

QStringList QLoaderCd::tab(const QStringList &/*arguments*/)
{
    return {};
}
