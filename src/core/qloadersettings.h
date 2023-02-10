// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSETTINGS_H
#define QLOADERSETTINGS_H

#include "qtloaderglobal.h"
#include <QVariant>

class QLoaderTreePrivate;
class QLoaderTree;

class QLoaderBlob
{
public:
    QByteArray array;
    QDataStream::Version version{QDataStream::Qt_6_0};
};

class Q_LOADER_EXPORT QLoaderSettings
{
    Q_GADGET

    QLoaderSettings *const q_ptr;
    QLoaderTreePrivate *const d_ptr;

protected:
    explicit QLoaderSettings(QLoaderSettings *settings);

    bool addBlob(const QString &key);
    void emitError(const QString &error) const;
    void emitInfo(const QString &info) const;
    void emitWarning(const QString &warning) const;
    virtual QVariant fromString(const QString &value) const;
    virtual QString fromVariant(const QVariant &variant) const;
    bool removeBlob(const QString &key);
    bool setValue(const QString &key, const QVariant &value);

public:
    enum Key
    {
        No,
        Value,
        Blob
    };

    QLoaderSettings(QLoaderTreePrivate &d);
    virtual ~QLoaderSettings();

    QLoaderBlob blob(const QString &key) const;
    QByteArray className() const;
    QLoaderSettings::Key contains(const QString &key) const;
    void dumpSettingsTree() const;
    virtual bool isCopyable(const QStringList &to) const;
    virtual bool isMovable(const QStringList &to) const;
    virtual QLoaderBlob saveBlob(const QString &key) const;
    QStringList section() const;
    QLoaderTree *tree() const;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
};

#endif // QLOADERSETTINGS_H
