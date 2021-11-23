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

#ifndef QLOADERSETTINGS_H
#define QLOADERSETTINGS_H

#include "qtloaderglobal.h"
#include <QObject>

class QLoaderSettingsPrivate;
class QLoaderTreePrivate;
class QLoaderTree;

class Q_LOADER_EXPORT QLoaderSettings
{
    Q_DISABLE_COPY_MOVE(QLoaderSettings)

protected:
    const QScopedPointer<QLoaderSettingsPrivate> d_ptr;

    explicit QLoaderSettings(QLoaderSettings *settings);

    bool contains(const QString &key) const;
    QStringList section() const;
    void setValue(const QString &key, const QVariant &value);
    QLoaderTree *tree() const;
    QVariant value(const QString &key) const;

public:
    explicit QLoaderSettings(QLoaderTreePrivate *objectTreePrivate);
    virtual ~QLoaderSettings();

    const char *className() const;
};
Q_DECLARE_INTERFACE(QLoaderSettings, "QLoaderSettings")

#endif // QLOADERSETTINGS_H
