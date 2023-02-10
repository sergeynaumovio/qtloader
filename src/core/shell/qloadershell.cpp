// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include "qloadercommandinterface.h"
#include "qloadertree.h"
#include <QPlainTextEdit>

class QLoaderShellPrivate
{
public:
    QHash<QString, QObject *> commands;
    QStringList home;
    QStringList section;
    QLoaderTerminalInterface *terminal{};
};

QLoaderShell::QLoaderShell(QLoaderSettings *settings)
:   QLoaderSettings(settings),
    d_ptr(new QLoaderShellPrivate)
{
    d_ptr->home = value("home", section()).toStringList();
    d_ptr->section = d_ptr->home;
}

QLoaderShell::~QLoaderShell()
{ }

void QLoaderShell::addCommand(QObject *object)
{
    QLoaderCommandInterface *command = qobject_cast<QLoaderCommandInterface *>(object);
    if (!command)
        emitError("interface not valid");
    else if (d_ptr->commands.contains(command->name()))
        emitError("command \"" + command->name() + "\" already set");
    else
    {
        d_ptr->commands.insert(command->name(), object);
    }
}

bool QLoaderShell::cd(const QString &relative)
{
    QStringList absolute = d_ptr->section;
    for (const QString &name : relative.split('/'))
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
            d_ptr->terminal->out()->insertPlainText("\nshell: " + name + ": " + error.message);

        return error;
    }

    QLoaderError error{.status = QLoaderError::Object, .message = "command not found"};
    if (d_ptr->terminal)
        d_ptr->terminal->out()->insertPlainText("\nshell: " + name + ": " + error.message);

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
