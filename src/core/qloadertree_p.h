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

#ifndef QLOADERTREE_P_H
#define QLOADERTREE_P_H

#include "qloadertree.h"
#include <QStringList>
#include <QHash>
#include <QVariant>

class QLoaderSettings;
class QLoaderTree;
class QFile;

struct QLoaderSettingsData
{
    QLoaderSettings *parent{};
    QStringList section;
    QByteArray className;
    int classLine;
    QMap<QString, QVariant> properties;
    std::vector<QLoaderSettings*> children;
};

class QLoaderTreePrivate
{
    void setProperties(QLoaderSettings *settings, QObject *object);
    void load(QLoaderSettings *settings, QObject *parent);

public:
    QLoaderTree *const q_ptr;
    QFile *file{};
    QLoaderTree::Status status{};
    QString error;
    int errorLine{-1};
    bool loaded{};
    bool modified{};

    struct
    {
        QLoaderSettings *settings{};
        QObject *object{};

    } root;

    struct
    {
        QHash<QStringList, QLoaderSettings*> settings;
        QHash<QLoaderSettings*, QLoaderSettingsData> data;

    } hash;

    QLoaderTreePrivate(const QString &fileName, QLoaderTree *q);
    virtual ~QLoaderTreePrivate();

    QObject *builtin(QLoaderSettings *settings, QObject *parent);
    QObject *external(QLoaderSettings *settings, QObject *parent);
    bool copy(const QStringList &section, const QStringList &to);
    bool load();
    bool move(const QStringList &section, const QStringList &to);
    bool save();
};

#endif // QLOADERTREE_P_H
