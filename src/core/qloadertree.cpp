/****************************************************************************
**
** Copyright (C) 2021, 2022 Sergey Naumov
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

#include "qloadertree.h"
#include "qloadertree_p.h"
#include <QFile>

QLoaderTree::QLoaderTree(QLoaderTreePrivate &d, QObject *parent)
:   QObject(parent),
    d_ptr(&d)
{ }

QLoaderTree::QLoaderTree(const QString &fileName, QObject *parent)
:   QObject(parent),
    d_ptr(new QLoaderTreePrivate(fileName, this))
{ }

QLoaderTree::~QLoaderTree()
{ }

bool QLoaderTree::contains(const QStringList &section) const
{
    return d_ptr->hash.settings.contains(section);
}

bool QLoaderTree::copy(const QStringList &section, const QStringList &to)
{
    return d_ptr->copyOrMove(section, to, Action::Copy);
}

QString QLoaderTree::errorMessage() const
{
    return d_ptr->errorMessage;
}

int QLoaderTree::errorLine() const
{
    return d_ptr->errorLine;
}

QObject *QLoaderTree::errorObject() const
{
    return d_ptr->errorObject;
}

QString QLoaderTree::fileName() const
{
    if (d_ptr->file)
        return d_ptr->file->fileName();

    return {};
}

QString QLoaderTree::infoMessage() const
{
    return d_ptr->infoMessage;
}

QObject *QLoaderTree::infoObject() const
{
    return d_ptr->infoObject;
}

bool QLoaderTree::isLoaded() const
{
    return d_ptr->loaded;
}

bool QLoaderTree::isModified() const
{
    return d_ptr->modified;
}

bool QLoaderTree::load() const
{
    return d_ptr->load();
}

bool QLoaderTree::move(const QStringList &section, const QStringList &to)
{
    return d_ptr->copyOrMove(section, to, Action::Move);
}

QObject *QLoaderTree::object(const QStringList &section) const
{
    if (d_ptr->hash.settings.contains(section))
        return d_ptr->hash.data[d_ptr->hash.settings[section]].object;

    return nullptr;
}

bool QLoaderTree::save() const
{
    return d_ptr->save();
}

QLoaderTree::Status QLoaderTree::status() const
{
    return d_ptr->status;
}

QString QLoaderTree::warningMessage() const
{
    return d_ptr->warningMessage;
}

QObject *QLoaderTree::warningObject() const
{
    return d_ptr->warningObject;
}
