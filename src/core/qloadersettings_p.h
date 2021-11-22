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
struct QLoaderTreePrivate;
class QLoaderTree;

struct QLoaderSettingsPrivate
{
    QLoaderSettings *q_ptr;
    QLoaderTreePrivate *d_tree_ptr;

    QLoaderSettingsPrivate(QLoaderSettings *q, QLoaderTreePrivate *d_tree);
    ~QLoaderSettingsPrivate();

    QStringList section();
    bool contains(const QString &key);
    QVariant value(const QString &key);
    void setValue(const QString &key, const QVariant &value);
    const char *className();
};

#endif // QLOADERSETTINGS_P_H

