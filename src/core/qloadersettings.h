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

#ifndef QLOADERSETTINGS_H
#define QLOADERSETTINGS_H

#include "qtloaderglobal.h"
#include <QVariant>

class QLoaderTreePrivate;
class QLoaderTree;

class Q_LOADER_EXPORT QLoaderSettings
{
    Q_DISABLE_COPY_MOVE(QLoaderSettings)

    QLoaderSettings *const q_ptr;
    QLoaderTreePrivate *const d_ptr;

protected:
    explicit QLoaderSettings(QLoaderSettings *settings);

    bool contains(const QString &key) const;
    void emitError(const QString &error);
    void emitInfo(const QString &info);
    void emitWarning(const QString &warning);
    virtual QVariant fromString(const QString &value) const;
    virtual QString fromVariant(const QVariant &variant) const;
    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

public:
    explicit QLoaderSettings(QLoaderTreePrivate *d);
    virtual ~QLoaderSettings();

    const char *className() const;
    void dumpSettingsTree() const;
    const QStringList section() const;
    QLoaderTree *tree() const;
};

#endif // QLOADERSETTINGS_H
