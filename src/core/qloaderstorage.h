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

#ifndef QLOADERSTORAGE_H
#define QLOADERSTORAGE_H

#include "qloadersettings.h"
#include "qloadersaveinterface.h"

class QLoaderStoragePrivate;

class QLoaderStorage : public QObject, public QLoaderSettings
{
    Q_OBJECT

    QScopedPointer<QLoaderStoragePrivate> d_ptr;

public:
    QLoaderStorage(QLoaderSettings *settings, QObject *parent);
    ~QLoaderStorage();
};

#endif // QLOADERSTORAGE_H
