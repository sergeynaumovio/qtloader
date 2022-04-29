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

#ifndef QLOADERSHELL_H
#define QLOADERSHELL_H

#include "qloadersettings.h"
#include "qloadererror.h"

class QLoaderShellPrivate;
class QLoaderTerminalInterface;

class QLoaderCommand
{
public:
    QString name;
    QStringList arguments;
};

using QLoaderCommandList = QList<QLoaderCommand>;

class Q_LOADER_EXPORT QLoaderShell : public QObject, public QLoaderSettings
{
    Q_OBJECT

    friend class QLoaderTreePrivate;
    QScopedPointer<QLoaderShellPrivate> d_ptr;

    QLoaderShell(QLoaderSettings *settings);

Q_SIGNALS:
    void finished(QString name, QLoaderError error);
    void started(QString name, QStringList arguments);

public:
    ~QLoaderShell();

    void addCommand(QObject *command);
    bool cd(const QString &objectName);
    bool cdUp();
    QLoaderError error() const;
    QLoaderError exec(const QLoaderCommand &command);
    QLoaderError exec(const QLoaderCommandList &pipeline);
    QLoaderError exec(const QString &name, const QStringList &arguments);
    QStringList section() const;
    void setTerminal(QLoaderTerminalInterface *terminal);
    QLoaderTerminalInterface *terminal() const;
};

#endif // QLOADERSHELL_H
