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

#include "qloadersettings.h"
#include "qloadertree_p.h"

QLoaderSettings::QLoaderSettings(QLoaderSettings *settings)
:   q_ptr(settings->q_ptr),
    d_ptr(settings->d_ptr)
{
    d_ptr->hash.data[q_ptr].settings = this;
}

QLoaderSettings::QLoaderSettings(QLoaderTreePrivate &d)
:   q_ptr(this),
    d_ptr(&d)
{ }

QLoaderSettings::~QLoaderSettings()
{
    if (q_ptr != this)
    {
        QHash<QLoaderSettings*, QLoaderSettingsData> &data = d_ptr->hash.data;
        const QLoaderSettingsData &item = data[q_ptr];

        if (data.contains(item.parent))
            std::erase(data[item.parent].children, q_ptr);

        d_ptr->hash.settings.remove(item.section);
        data.remove(q_ptr);

        d_ptr->modified = true;
        emit d_ptr->q_ptr->settingsChanged();
    }
}

bool QLoaderSettings::contains(const QString &key) const
{
    return d_ptr->hash.data[q_ptr].properties.contains(key);;
}

void QLoaderSettings::emitError(const QString &error)
{
    QLoaderTree::Status status = QLoaderTree::ObjectError;
    d_ptr->status = status;
    d_ptr->errorMessage = error;
    QObject *object = d_ptr->hash.data[q_ptr].object;
    if (object)
    {
        d_ptr->errorObject = object;
        emit d_ptr->q_ptr->statusChanged(status);
        emit d_ptr->q_ptr->errorChanged(object, error);
    }
}

void QLoaderSettings::emitInfo(const QString &info)
{
    d_ptr->infoMessage = info;
    d_ptr->infoChanged = true;
    QObject *object = d_ptr->hash.data[q_ptr].object;
    if (object)
    {
        d_ptr->infoObject = object;
        emit d_ptr->q_ptr->infoChanged(object, info);
    }
}

void QLoaderSettings::emitWarning(const QString &warning)
{
    d_ptr->warningMessage = warning;
    d_ptr->warningChanged = true;
    QObject *object = d_ptr->hash.data[q_ptr].object;
    if (object)
    {
        d_ptr->warningObject = object;
        emit d_ptr->q_ptr->warningChanged(object, warning);
    }
}

QVariant QLoaderSettings::fromString(const QString &value) const
{
    return d_ptr->fromString(value);
}

QString QLoaderSettings::fromVariant(const QVariant &variant) const
{
    return d_ptr->fromVariant(variant);
}

void QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->hash.data[q_ptr].properties[key] = fromVariant(value);
    d_ptr->modified = true;
    emit d_ptr->q_ptr->settingsChanged();
}

QVariant QLoaderSettings::value(const QString &key, const QVariant &defaultValue) const
{
    const QMap<QString, QString> &properties = d_ptr->hash.data[q_ptr].properties;
    if (properties.contains(key))
        return fromString(properties[key]);

    return defaultValue;
}

const char *QLoaderSettings::className() const
{
    return d_ptr->hash.data[q_ptr].className.data();
}

void QLoaderSettings::dumpSettingsTree() const
{
    d_ptr->dump(q_ptr);
}

const QStringList &QLoaderSettings::section() const
{
    return d_ptr->hash.data[q_ptr].section;
}

QLoaderTree *QLoaderSettings::tree() const
{
    return d_ptr->q_ptr;
}
