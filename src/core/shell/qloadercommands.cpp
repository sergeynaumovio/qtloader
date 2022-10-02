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

#include "qloadercommands.h"
#include "qloadercommandinterface.h"
#include "qloadershell_p.h"
#include "qloadershell.h"
#include "qloaderclear.h"

QLoaderCommands::QLoaderCommands(QLoaderShellPrivate &d)
:   d_ptr(&d)
{
    addCommand(new QLoaderClear(d_ptr->q_ptr));
}

void QLoaderCommands::addCommand(QObject *object)
{
    QLoaderCommandInterface *command = qobject_cast<QLoaderCommandInterface *>(object);
    if (!command)
        d_ptr->emitError("interface not valid");
    else if (commands.contains(command->name()))
        d_ptr->emitError("command \"" + command->name() + "\" already set");
    else
        commands.insert(command->name(), object);
}

QLoaderError QLoaderCommands::exec(const QString &command,
                                   const QStringList &section,
                                   const QStringList &arguments)
{
    if (commands.contains(command) && d_ptr->q_ptr->children().contains(commands[command]))
        return qobject_cast<QLoaderCommandInterface *>(commands[command])->exec(arguments);

    return {.status = QLoaderError::Object, .message = "command not found"};
}
