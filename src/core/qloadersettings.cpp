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
#include "qloadertree_p.h"

QLoaderSettings::QLoaderSettings(QLoaderSettings *settings)
:   q_ptr(settings->q_ptr),
    d_ptr(settings->d_ptr)
{
    delete settings;
}

QLoaderSettings::QLoaderSettings(QLoaderTreePrivate *d)
:   q_ptr(this),
    d_ptr(d)
{ }

QLoaderSettings::~QLoaderSettings()
{
    if (q_ptr != this)
    {
        QStringList section = d_ptr->hash.data[q_ptr].section;
        QLoaderSettings *settings = d_ptr->hash.settings[section];
        d_ptr->hash.settings.remove(section);
        d_ptr->hash.data.remove(settings);
        d_ptr->modified = true;
        emit d_ptr->q_ptr->settingsChanged();
    }
}

bool QLoaderSettings::contains(const QString &key) const
{
    return d_ptr->hash.data[q_ptr].properties.contains(key);;
}

void QLoaderSettings::dumpSettingsTree() const
{
    d_ptr->dump(q_ptr);
}

const QStringList QLoaderSettings::section() const
{
    return d_ptr->hash.data[q_ptr].section;
}

void QLoaderSettings::setObjectError(const QString &error)
{
    d_ptr->status = QLoaderTree::ObjectError;
    d_ptr->error = error;
}

void QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->hash.data[q_ptr].properties[key] = value;
    d_ptr->modified = true;
    emit d_ptr->q_ptr->settingsChanged();
}

QLoaderTree *QLoaderSettings::tree() const
{
    return d_ptr->q_ptr;
}

QVariant QLoaderSettings::value(const QString &key, const QVariant &defaultValue) const
{
    const QMap<QString, QVariant> &properties = d_ptr->hash.data[q_ptr].properties;
    if (properties.contains(key))
        return properties[key];

    return defaultValue;
}

const char *QLoaderSettings::className() const
{
    return d_ptr->hash.data[q_ptr].className.data();
}
