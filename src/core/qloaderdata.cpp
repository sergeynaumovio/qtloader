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

#include "qloaderdata.h"
#include "qloaderdatablob.h"
#include "qloaderdatainterface.h"

class QLoaderDataPrivate
{
public:
    QLoaderDataBlob *blob;
};

QLoaderData::QLoaderData(QLoaderSettings *settings, QObject *parent)
:   QObject(parent),
    QLoaderSettings(settings),
    d_ptr(new QLoaderDataPrivate)
{
    if (!qobject_cast<QLoaderDataInterface*>(parent))
    {
        emitError("QLoaderDataInterface not found");
        return;
    }

}

QLoaderData::~QLoaderData()
{ }

void QLoaderData::setBlob(QLoaderDataBlob *blob)
{
    d_ptr->blob = blob;
}
