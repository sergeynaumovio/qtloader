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

#ifndef QLOADERCOMMANDINTERFACE_H
#define QLOADERCOMMANDINTERFACE_H

#include "qloadererror.h"
#include <QObject>

class QLoaderCommandInterface
{
public:
    virtual QLoaderError exec(const QStringList &arguments) = 0;
    virtual QString name() const = 0;
    virtual QStringList tab(const QStringList &arguments) = 0;
};
Q_DECLARE_INTERFACE(QLoaderCommandInterface, "QLoaderCommandInterface")

#endif // QLOADERCOMMANDINTERFACE_H
