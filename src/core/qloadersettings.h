// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSETTINGS_H
#define QLOADERSETTINGS_H

#include "qtloaderglobal.h"
#include <QVariant>

class QLoaderTreePrivate;
class QLoaderTree;

class Q_LOADER_EXPORT QLoaderSettings
{
    Q_GADGET

    QLoaderSettings *const q_ptr;
    QLoaderTreePrivate *const d_ptr;

protected:
    QLoaderSettings(QObject *object, QLoaderSettings *settings);

    void emitError(const QString &error) const;
    void emitInfo(const QString &info) const;
    void emitWarning(const QString &warning) const;
    virtual QVariant fromString(const QString &value) const;
    virtual QString fromVariant(const QVariant &variant) const;
    bool setValue(const QString &key, const QVariant &value);

public:
    QLoaderSettings(QLoaderTreePrivate &d);
    virtual ~QLoaderSettings();

    QByteArray className() const;
    bool contains(const QString &key) const;
    void dumpSettingsTree() const;
    virtual bool isCopyable(const QStringList &to) const;
    virtual bool isMovable(const QStringList &to) const;
    QStringList section() const;
    QLoaderTree *tree() const;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
};

#endif // QLOADERSETTINGS_H
