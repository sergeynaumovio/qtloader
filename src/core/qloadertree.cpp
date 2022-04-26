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

QLoaderError QLoaderTree::backup()
{
    return {};
}

bool QLoaderTree::contains(const QStringList &section) const
{
    d_ptr->mutex.lock();
    bool containsSection = d_ptr->hash.settings.contains(section);
    d_ptr->mutex.unlock();

    return containsSection;
}

QLoaderError QLoaderTree::copy(const QStringList &section, const QStringList &to)
{
    return d_ptr->copy(section, to);
}

QLoaderData *QLoaderTree::data() const
{
    return d_ptr->data();
}

QString QLoaderTree::fileName() const
{
    if (d_ptr->file)
        return d_ptr->file->fileName();

    return {};
}

bool QLoaderTree::isModified() const
{
    return d_ptr->modified;
}

QLoaderError QLoaderTree::load() const
{
    return d_ptr->load();
}

QLoaderError QLoaderTree::move(const QStringList &section, const QStringList &to)
{
    return d_ptr->move(section, to);
}

QObject *QLoaderTree::object(const QStringList &section) const
{
    QObject *object{};
    d_ptr->mutex.lock();
    if (d_ptr->hash.settings.contains(section))
        object = d_ptr->hash.data[d_ptr->hash.settings[section]].object;
    d_ptr->mutex.unlock();

    return object;
}

QLoaderError QLoaderTree::save() const
{
    QLoaderError error;
    if (d_ptr->isSaving())
    {
        error.status = QLoaderError::Access;
        error.message = "saving in progress";
    }
    else
    {
        d_ptr->mutex.lock();
        error = d_ptr->save();
        d_ptr->mutex.unlock();
    }

    return error;
}

QLoaderShell *QLoaderTree::shell() const
{
    return d_ptr->shell();
}
