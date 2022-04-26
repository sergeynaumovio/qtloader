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

class Q_LOADER_EXPORT QLoaderShell : public QObject, public QLoaderSettings
{
    Q_OBJECT

    friend class QLoaderShellPrivate;
    QScopedPointer<QLoaderShellPrivate> d_ptr;

public:
    QLoaderShell(QLoaderSettings *settings, QObject *parent);
    ~QLoaderShell();

    void addCommand(QObject *command);
    QLoaderError exec(const QString &command, const QStringList &arguments);
};

#endif // QLOADERSHELL_H
