// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadershell.h"
#include "qloadercommandinterface.h"
#include "qloadertree.h"
#include <QHash>
#include <QPlainTextEdit>

using namespace Qt::Literals::StringLiterals;

class QLoaderShellPrivate
{
public:
    QHash<QString, QObject *> commands;
    QString home;
    QString section;
};

QLoaderShell::QLoaderShell(QLoaderSettings *settings)
:   QLoaderSettings(this, settings),
    d_ptr(new QLoaderShellPrivate)
{
    d_ptr->home = value(u"home"_s, section()).toString();

    if (!tree()->contains(d_ptr->home))
    {
        emitWarning(u'[' + d_ptr->home + u']' + u": home section not valid"_s);
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
    QString absolute = d_ptr->section;
    for (QStringView name : QStringTokenizer{relative, u'/'})
        absolute.append(u'/' + name.toString());

    if (tree()->contains(absolute))
    {
        d_ptr->section = absolute;

        return true;
    }

    return false;
}

void QLoaderShell::cdHome()
{
    d_ptr->section = d_ptr->home;
}

bool QLoaderShell::cdUp()
{
    int level = d_ptr->section.count(u'/') + 1;
    if (level > 1)
    {
        int splitIndex = d_ptr->section.lastIndexOf(u'/', -2);
        d_ptr->section = d_ptr->section.first(splitIndex);

        return true;
    }

    return false;
}

QLoaderError QLoaderShell::exec(QLatin1StringView command)
{
    return exec(command, {});
}

QLoaderError QLoaderShell::exec(const QString &name, const QStringList &arguments)
{
    if (d_ptr->commands.contains(name))
        return qobject_cast<QLoaderCommandInterface *>(d_ptr->commands[name])->exec(arguments);

    return QLoaderError{.status = QLoaderError::Object, .message = u"command not found"_s};
}

QString QLoaderShell::section() const
{
    return d_ptr->section;
}
