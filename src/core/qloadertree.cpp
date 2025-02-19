// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadertree.h"
#include "qloadertree_p.h"
#include <QFile>

using namespace Qt::Literals::StringLiterals;

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

bool QLoaderTree::contains(QStringView section) const
{
    d_ptr->mutex.lock();
    bool containsSection = d_ptr->hash.settings.sections.contains(section);
    d_ptr->mutex.unlock();

    return containsSection;
}

QLoaderError QLoaderTree::copy(QStringView section, QStringView to)
{
    return d_ptr->copy(section, to);
}

QString QLoaderTree::fileName() const
{
    if (d_ptr->file)
        return d_ptr->file->fileName();

    return {};
}

bool QLoaderTree::isLoaded() const
{
    return d_ptr->loaded;
}

bool QLoaderTree::isModified() const
{
    return d_ptr->modified;
}

QLoaderError QLoaderTree::load() const
{
    return d_ptr->load();
}

QLoaderError QLoaderTree::move(QStringView section, QStringView to)
{
    return d_ptr->move(section, to);
}

QLoaderShell *QLoaderTree::newShellInstance() const
{
    return d_ptr->newShellInstance();
}

QObject *QLoaderTree::object(QStringView section) const
{
    QObject *object{};
    d_ptr->mutex.lock();
    if (d_ptr->hash.settings.sections.contains(section))
        object = d_ptr->hash.data.value(d_ptr->hash.settings.sections.value(section)).object;
    d_ptr->mutex.unlock();

    return object;
}

QLoaderError QLoaderTree::save() const
{
    QLoaderError error;
    if (d_ptr->isSaving())
    {
        error.status = QLoaderError::Access;
        error.message = u"saving in progress"_s;
    }
    else
    {
        d_ptr->mutex.lock();
        error = d_ptr->save();
        d_ptr->mutex.unlock();
    }

    return error;
}

QLoaderSettings *QLoaderTree::settings(QObject *object) const
{
    QLoaderSettings *settings{};
    d_ptr->mutex.lock();
    if (d_ptr->hash.settings.objects.contains(object))
        settings = d_ptr->hash.settings.objects.value(object);
    d_ptr->mutex.unlock();

    return settings;
}
