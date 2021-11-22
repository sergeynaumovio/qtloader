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
{
    d_tree_ptr->hash.remove(q_ptr);
}

QStringList QLoaderSettingsPrivate::section()
{
    return d_tree_ptr->hash[q_ptr].section;
}

const char *QLoaderSettingsPrivate::className()
{
    return d_tree_ptr->hash[q_ptr].className.data();
}

bool QLoaderSettingsPrivate::contains(const QString &key)
{
    return d_tree_ptr->hash[q_ptr].properties.contains(key);
}

QVariant QLoaderSettingsPrivate::value(const QString &key)
{
    return d_tree_ptr->hash[q_ptr].properties[key];
}

void QLoaderSettingsPrivate::setValue(const QString &key, const QVariant &value)
{
    d_tree_ptr->hash[q_ptr].properties[key] = value;
}
