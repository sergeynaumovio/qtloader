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

#include "qloadersettings_p.h"
#include "qloadertree_p.h"

QLoaderSettingsPrivate::QLoaderSettingsPrivate(QLoaderSettings *q,
                                               QLoaderTreePrivate *d_tree)
:   q_ptr(q),
    d_tree_ptr(d_tree)
{ }

QLoaderSettingsPrivate::~QLoaderSettingsPrivate()
{ }

const char *QLoaderSettingsPrivate::className() const
{
    return d_tree_ptr->hash.data[q_ptr].className.data();
}

bool QLoaderSettingsPrivate::contains(const QString &key) const
{
    return d_tree_ptr->hash.data[q_ptr].properties.contains(key);
}

QStringList QLoaderSettingsPrivate::section() const
{
    return d_tree_ptr->hash.data[q_ptr].section;
}

void QLoaderSettingsPrivate::setValue(const QString &key, const QVariant &value)
{
    d_tree_ptr->hash.data[q_ptr].properties[key] = value;
    d_tree_ptr->modified = true;
    emit d_tree_ptr->q_ptr->settingsChanged();
}

QVariant QLoaderSettingsPrivate::value(const QString &key, const QVariant &defaultValue) const
{
    const QMap<QString, QVariant> &properties = d_tree_ptr->hash.data[q_ptr].properties;
    if (properties.contains(key))
        return properties[key];

    return defaultValue;
}
