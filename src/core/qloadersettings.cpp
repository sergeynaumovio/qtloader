// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadersettings.h"
#include "qloadertree_p.h"

using namespace Qt::Literals::StringLiterals;

QLoaderSettings::QLoaderSettings(QObject *object, QLoaderSettings *settings)
:   q_ptr(settings->q_ptr),
    d_ptr(settings->d_ptr)
{
    d_ptr->mutex.lock();
    if (!d_ptr->hash.data[q_ptr].object)
    {
        d_ptr->hash.data[q_ptr].object = object;
        d_ptr->hash.settings.objects[object] = this;
        d_ptr->setProperties(d_ptr->hash.data[q_ptr], object);
    }
    d_ptr->hash.data[q_ptr].settings.append(this);
    d_ptr->mutex.unlock();
}

QLoaderSettings::QLoaderSettings(QLoaderTreePrivate &d)
:   q_ptr(this),
    d_ptr(&d)
{ }

QLoaderSettings::~QLoaderSettings()
{
    if (q_ptr != this)
    {
        bool removeLastInstance{};
        d_ptr->mutex.lock();
        {
            QHash<QLoaderSettings*, QLoaderSettingsData> &data = d_ptr->hash.data;
            data[q_ptr].settings.removeOne(this);
            const QLoaderSettingsData &item = data[q_ptr];

            if ((removeLastInstance = item.settings.isEmpty()))
            {
                if (data.contains(item.parent))
                    data[item.parent].children.removeOne(q_ptr);

                d_ptr->hash.settings.sections.remove(item.section);
                d_ptr->hash.settings.objects.remove(item.object);
                data.remove(q_ptr);
                d_ptr->modified = true;

                delete q_ptr;
            }
        }
        d_ptr->mutex.unlock();

        if (removeLastInstance)
            emit d_ptr->q_ptr->settingsChanged();
    }
}

bool QLoaderSettings::addBlob(const QString &key)
{
    if (contains(key))
    {
        emitError(u"key \""_s + key + u"\" already set"_s);
        return false;
    }

    QUuid uuid = d_ptr->createStorageUuid();
    if (uuid.isNull())
    {
        emitError(u"storage object not set"_s);
        return false;
    }

    d_ptr->mutex.lock();
    QLoaderProperty value {.isBlob = true,
                           .isValue = false,
                           .string = u"QLoaderBlob("_s + uuid.toString(QUuid::WithoutBraces) + u")"_s};

    d_ptr->hash.data[q_ptr].properties.insert(key, value);
    d_ptr->mutex.unlock();

    return true;
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

bool QLoaderSettings::removeBlob(const QString &key)
{
    d_ptr->mutex.lock();
    bool contains = d_ptr->hash.data[q_ptr].properties.contains(key);
    if (contains && (contains = d_ptr->hash.data[q_ptr].properties[key].isBlob))
         d_ptr->hash.data[q_ptr].properties.remove(key);

    d_ptr->mutex.unlock();

    return contains;
}

bool QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->mutex.lock();
    bool contains = d_ptr->hash.data[q_ptr].properties.contains(key);
    if (!contains || d_ptr->hash.data[q_ptr].properties[key].isValue)
    {
        if (contains && value.isNull())
            d_ptr->hash.data[q_ptr].properties.remove(key);
        else if (!value.isNull())
            d_ptr->hash.data[q_ptr].properties[key] = fromVariant(value);

        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->settingsChanged();
        return true;
    }
    d_ptr->mutex.unlock();

    return false;
}

QLoaderBlob QLoaderSettings::blob(const QString &key) const
{
    d_ptr->mutex.lock();
    QLoaderBlob bo;
    if (d_ptr->hash.data[q_ptr].properties.contains(key))
    {
        QLoaderProperty property = d_ptr->hash.data[q_ptr].properties[key];
        if (property.isBlob)
        {
            QUuid uuid(fromString(property).toByteArray());
            if (!uuid.isNull())
                bo = d_ptr->blob(uuid);
        }
    }
    d_ptr->mutex.unlock();

    return bo;
}

QLoaderSettings::Key QLoaderSettings::contains(const QString &key) const
{
    QMutexLocker locker(&d_ptr->mutex);
    if (!d_ptr->hash.data[q_ptr].properties.contains(key))
        return QLoaderSettings::No;

    if (d_ptr->hash.data[q_ptr].properties[key].isBlob)
        return QLoaderSettings::Blob;

    return QLoaderSettings::Value;
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

bool QLoaderSettings::isCopyable(const QStringList &) const
{
    return {};
}

bool QLoaderSettings::isMovable(const QStringList &) const
{
    return {};
}

QLoaderBlob QLoaderSettings::saveBlob(const QString &/*key*/) const
{
    return {};
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
    const QMap<QString, QLoaderProperty> &properties = d_ptr->hash.data[q_ptr].properties;
    if (properties.contains(key))
        variant = fromString(properties[key]);
    d_ptr->mutex.unlock();

    return variant;
}
