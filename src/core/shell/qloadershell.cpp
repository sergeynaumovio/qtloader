// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include "qloadercommandinterface.h"
#include "qloadertree.h"
#include <QPlainTextEdit>

using namespace Qt::Literals::StringLiterals;

class QLoaderShellPrivate
{
public:
    QHash<QString, QObject *> commands;
    QStringList home;
    QStringList section;
    QLoaderTerminalInterface *terminal{};
};

QLoaderShell::QLoaderShell(QLoaderSettings *settings)
:   QLoaderSettings(this, settings),
    d_ptr(new QLoaderShellPrivate)
{
    QString home = value(u"home"_s, section()).toString();
    d_ptr->home = value(u"home"_s, section()).toString().split(u'/');

    if (!tree()->contains(d_ptr->home))
    {
        emitWarning(u'[' + home + u']' + u": home section not valid"_s);
        d_ptr->home = section();
    }

    d_ptr->section = d_ptr->home;
}

QLoaderShell::~QLoaderShell()
{ }

void QLoaderShell::addCommand(QObject *object)
{
    QLoaderCommandInterface *command = qobject_cast<QLoaderCommandInterface *>(object);
    if (!command)
        emitError(u"interface not valid"_s);
    else if (d_ptr->commands.contains(command->name()))
        emitError(u"command \""_s + command->name() + u"\" already set"_s);
    else
    {
        d_ptr->commands.insert(command->name(), object);
    }
}

bool QLoaderShell::cd(const QString &relative)
{
    QStringList absolute = d_ptr->section;
    for (const QString &name : relative.split(u'/'))
        absolute.append(name);

    if (tree()->contains(absolute))
    {
        d_ptr->section = absolute;
        if (d_ptr->terminal)
            d_ptr->terminal->setCurrentSection(d_ptr->section);

        return true;
    }

    return false;
}

void QLoaderShell::cdHome()
{
    d_ptr->section = d_ptr->home;
    if (d_ptr->terminal)
        d_ptr->terminal->setCurrentSection(d_ptr->section);
}

bool QLoaderShell::cdUp()
{
    if (d_ptr->section.size() > 1)
    {
        d_ptr->section.removeLast();
        if (d_ptr->terminal)
            d_ptr->terminal->setCurrentSection(d_ptr->section);

        return true;
    }

    return false;
}

QLoaderError QLoaderShell::exec(const QString &name, const QStringList &arguments)
{
    if (d_ptr->commands.contains(name))
    {
        QLoaderError error = qobject_cast<QLoaderCommandInterface *>(d_ptr->commands[name])->exec(arguments);
        if (error && d_ptr->terminal)
            d_ptr->terminal->out()->insertPlainText(u"\nshell: "_s + name + u": "_s + error.message);

        return error;
    }

    QLoaderError error{.status = QLoaderError::Object, .message = u"command not found"_s};
    if (d_ptr->terminal)
        d_ptr->terminal->out()->insertPlainText(u"\nshell: "_s + name + u": "_s + error.message);

    return error;
}

QStringList QLoaderShell::section() const
{
    return d_ptr->section;
}

void QLoaderShell::setTerminal(QLoaderTerminalInterface *terminal)
{
    d_ptr->terminal = terminal;
}

QLoaderTerminalInterface *QLoaderShell::terminal() const
{
    return d_ptr->terminal;
}
