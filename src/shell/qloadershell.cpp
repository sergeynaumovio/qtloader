/****************************************************************************
**
** Copyright (C) 2022 Sergey Naumov
**
** Permission to use, copy, modify, and/or distribute this
** software for any purpose with or without fee is hereby granted.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
** CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
** LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
** NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
** CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**
****************************************************************************/

#include "qloadershell.h"
#include "qloaderterminalinterface.h"
#include "qloadercommandinterface.h"
#include "qloaderclear.h"
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
