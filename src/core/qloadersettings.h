// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
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

    friend class QLoaderTreePrivate;

    QLoaderSettings *const q_ptr;
    QLoaderTreePrivate *const d_ptr;

    QLoaderSettings(QLoaderTreePrivate &d);

public:
    enum LoadHint
    {
        LoadThisOnly = 0x0,
        LoadChildren = 0x1,
    };
    Q_DECLARE_FLAGS(LoadHints, LoadHint)
    Q_ENUM(LoadHint)
    Q_FLAG(LoadHints)

protected:
    QLoaderSettings(QObject *object, QLoaderSettings *settings, LoadHints = LoadChildren);

    void emitError(const QString &error) const;
    void emitInfo(const QString &info) const;
    void emitWarning(const QString &warning) const;
    virtual QVariant fromString(const QString &value) const;
    virtual QString fromVariant(const QVariant &variant) const;
    bool setValue(const QString &key, const QVariant &value);

public:
    virtual ~QLoaderSettings();

    const char *className() const;
    bool contains(const QString &key) const;
    void dumpSettingsTree() const;
    virtual bool isCopyable(QStringView to) const;
    virtual bool isMovable(QStringView to) const;
    QStringView section() const;
    QLoaderTree *tree() const;
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
};

#endif // QLOADERSETTINGS_H
