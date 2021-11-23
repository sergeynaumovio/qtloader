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

#ifndef QLOADERSETTINGS_P_H
#define QLOADERSETTINGS_P_H

#include <QStringList>
#include <QVariant>

class QLoaderSettings;
class QLoaderTreePrivate;
class QLoaderTree;

class QLoaderSettingsPrivate
{
public:
    QLoaderSettings *const q_ptr;
    QLoaderTreePrivate *d_tree_ptr;

    QLoaderSettingsPrivate(QLoaderSettings *q, QLoaderTreePrivate *d_tree);
    virtual ~QLoaderSettingsPrivate();

    QStringList section() const;
    bool contains(const QString &key) const;
    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &value);
    const char *className() const;
};

#endif // QLOADERSETTINGS_P_H

