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
{ }

QLoaderSettings::QLoaderSettings(QLoaderTreePrivate &d)
:   q_ptr(this),
    d_ptr(&d)
{ }

QLoaderSettings::~QLoaderSettings()
{
    if (q_ptr != this)
    {
        d_ptr->mutex.lock();
        {
            QHash<QLoaderSettings*, QLoaderSettingsData> &data = d_ptr->hash.data;
            const QLoaderSettingsData &item = data[q_ptr];

            if (data.contains(item.parent))
                std::erase(data[item.parent].children, q_ptr);

            d_ptr->hash.settings.remove(item.section);
            data.remove(q_ptr);

            d_ptr->modified = true;
        }
        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->settingsChanged();
    }
}

bool QLoaderSettings::addBlob(const QString &/*key*/)
{
    return {};
}

void QLoaderSettings::emitError(const QString &error) const
{
    if (d_ptr->loaded)
    {
        d_ptr->mutex.lock();
        QObject *object = d_ptr->hash.data[q_ptr].object;
        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->errorChanged(object, error);
    }
    else
    {
        d_ptr->mutex.lock();
        d_ptr->errorMessage = error;
        d_ptr->mutex.unlock();
    }
}

void QLoaderSettings::emitInfo(const QString &info) const
{
    if (d_ptr->loaded)
    {
        d_ptr->mutex.lock();
        QObject *object = d_ptr->hash.data[q_ptr].object;
        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->infoChanged(object, info);
    }
    else
    {
        d_ptr->mutex.lock();
        d_ptr->infoMessage = info;
        d_ptr->mutex.unlock();
    }
}

void QLoaderSettings::emitWarning(const QString &warning) const
{
    if (d_ptr->loaded)
    {
        d_ptr->mutex.lock();
        QObject *object = d_ptr->hash.data[q_ptr].object;
        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->warningChanged(object, warning);
    }
    else
    {
        d_ptr->mutex.lock();
        d_ptr->warningMessage = warning;
        d_ptr->mutex.unlock();
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

bool QLoaderSettings::removeBlob(const QString &/*key*/)
{
    return {};
}

QLoaderBlob QLoaderSettings::saveBlob(const QString &/*key*/) const
{
    return {};
}

bool QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->mutex.lock();
    d_ptr->hash.data[q_ptr].properties[key] = fromVariant(value);
    d_ptr->modified = true;
    d_ptr->mutex.unlock();
    emit d_ptr->q_ptr->settingsChanged();
    return true;
}

QLoaderBlob QLoaderSettings::blob(const QString &/*key*/) const
{
    return {};
}

QLoaderSettings::Key QLoaderSettings::contains(const QString &key) const
{
    d_ptr->mutex.lock();
    bool containsKeyValue = d_ptr->hash.data[q_ptr].properties.contains(key);
    d_ptr->mutex.unlock();

    if (containsKeyValue)
        return QLoaderSettings::Value;

    return QLoaderSettings::No;
}

QByteArray QLoaderSettings::className() const
{
    d_ptr->mutex.lock();
    QByteArray name = d_ptr->hash.data[q_ptr].className;
    d_ptr->mutex.unlock();

    return name;
}

void QLoaderSettings::dumpSettingsTree() const
{
    d_ptr->mutex.lock();
    d_ptr->dump(q_ptr);
    d_ptr->mutex.unlock();
}

QStringList QLoaderSettings::section() const
{
    d_ptr->mutex.lock();
    QStringList list = d_ptr->hash.data[q_ptr].section;
    d_ptr->mutex.unlock();

    return list;
}

QLoaderTree *QLoaderSettings::tree() const
{
    return d_ptr->q_ptr;
}

QVariant QLoaderSettings::value(const QString &key, const QVariant &defaultValue) const
{
    d_ptr->mutex.lock();
    QVariant variant = defaultValue;
    const QMap<QString, QString> &properties = d_ptr->hash.data[q_ptr].properties;
    if (properties.contains(key))
        variant = fromString(properties[key]);
    d_ptr->mutex.unlock();

    return variant;
}
