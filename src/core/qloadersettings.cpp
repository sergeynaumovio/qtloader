// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadersettings.h"
#include "qloadertree.h"
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
        if (d_ptr->hash.data[q_ptr].settings.removeOne(this))
        {
            QHash<QLoaderSettings*, QLoaderSettingsData> &data = d_ptr->hash.data;
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

bool QLoaderSettings::setValue(const QString &key, const QVariant &value)
{
    d_ptr->mutex.lock();
    bool contains = d_ptr->hash.data[q_ptr].properties.contains(key);
    QString string = fromVariant(value);
    if (!contains || d_ptr->hash.data[q_ptr].properties.value(key) != string)
    {
        if (contains && value.isNull())
            d_ptr->hash.data[q_ptr].properties.remove(key);
        else if (!value.isNull())
            d_ptr->hash.data[q_ptr].properties[key] = string;

        d_ptr->mutex.unlock();
        emit d_ptr->q_ptr->settingsChanged();
        return true;
    }
    d_ptr->mutex.unlock();

    return false;
}

bool QLoaderSettings::contains(const QString &key) const
{
    QMutexLocker locker(&d_ptr->mutex);
    return d_ptr->hash.data[q_ptr].properties.contains(key);
}

const char *QLoaderSettings::className() const
{
    d_ptr->mutex.lock();
    const char *name = d_ptr->hash.data[q_ptr].className.data();
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
    if (d_ptr->hash.data[q_ptr].properties.contains(key))
        variant = fromString(d_ptr->hash.data[q_ptr].properties.value(key));
    d_ptr->mutex.unlock();

    return variant;
}
