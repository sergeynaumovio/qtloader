// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershellcd.h"
#include "qloadershell.h"
#include <QCommandLineParser>

using namespace Qt::Literals::StringLiterals;

class QLoaderShellCdPrivate
{
public:
    QLoaderShell *const shell;
    QCommandLineParser parser;

    QLoaderShellCdPrivate(QLoaderShell *sh)
    :   shell(sh)
    {
        parser.addPositionalArgument(u"object"_s, u"Change current object."_s);
    }
};

QLoaderShellCd::QLoaderShellCd(QLoaderSettings *settings, QLoaderShell *parent)
:   QObject(parent),
    QLoaderSettings(this, settings),
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
        if (posargs.first() == '.'_L1)
            return {};

        if (posargs.first() == ".."_L1)
        {
            d_ptr->shell->cdUp();
            return {};
        }

        if (d_ptr->shell->cd(posargs.first()))
            return {};

        return {.status = QLoaderError::Object, .message = posargs.first() + u": no such object"_s};
    }

    return {.status = QLoaderError::Object, .message = u"too many arguments"_s};
}

QString QLoaderShellCd::name() const
{
    return u"cd"_s;
}

QStringList QLoaderShellCd::tab(const QStringList &/*arguments*/)
{
    return {};
}
