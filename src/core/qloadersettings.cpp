/****************************************************************************
**
** Copyright (C) 2021 Sergey Naumov
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

#include "qloadersettings.h"
#include "qloadersettings_p.h"
#include "qloadertree_p.h"

QLoaderSettings::QLoaderSettings(QLoaderTreePrivate *objectTreePrivate)
:   d_ptr(new QLoaderSettingsPrivate(this, objectTreePrivate))
{ }

QLoaderSettings::~QLoaderSettings()
{
    if (d_ptr->q_ptr != this)
    {
        QStringList section = d_ptr->d_tree_ptr->hash.data[d_ptr->q_ptr].section;
        QLoaderSettings *settings = d_ptr->d_tree_ptr->hash.settings[section];
        d_ptr->d_tree_ptr->hash.settings.remove(section);
        d_ptr->d_tree_ptr->hash.data.remove(settings);
    }
}

QLoaderSettings::QLoaderSettings(QLoaderSettings *settings)
:   d_ptr(new QLoaderSettingsPrivate(settings->d_ptr->q_ptr, settings->d_ptr->d_tree_ptr))
{
    delete settings;
}

QLoaderTree *QLoaderSettings::tree() const
{
    return d_ptr->d_tree_ptr->q_ptr;
}

QStringList QLoaderSettings::section() const
{
    return d_ptr->section();
}

const char *QLoaderSettings::className() const
{
    return d_ptr->className();
}

bool QLoaderSettings::contains(const QString &key) const
{
    return d_ptr->contains(key);
}

QVariant QLoaderSettings::value(const QString &key) const
{
    return d_ptr->value(key);
}

void QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->setValue(key, value);
}

