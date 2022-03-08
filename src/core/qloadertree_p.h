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

#ifndef QLOADERTREE_P_H
#define QLOADERTREE_P_H

#include "qloadertree.h"
#include <QStringList>
#include <QMap>
#include <QHash>

class QLoaderSettings;
class QLoaderTree;
class QFile;
class QTextStream;
class QLoaderTreePrivateData;
class QLoaderTreeSection;

enum class Action { Copy, Move };

struct QLoaderSettingsData
{
    QLoaderSettings *parent{};
    QStringList section;
    int sectionLine;
    QByteArray className;
    QObject *object{};
    QMap<QString, QString> properties;
    std::vector<QLoaderSettings*> children;

    void clear();
};

class QLoaderTreePrivate
{
    QLoaderTreePrivateData &d;
    std::aligned_storage_t<104, sizeof (ptrdiff_t)> d_storage;

    void copyOrMoveRecursive(QLoaderSettings *settings,
                             const QLoaderTreeSection &src,
                             const QLoaderTreeSection &dst,
                             Action action);
    void dumpRecursive(QLoaderSettings *settings) const;
    void loadRecursive(QLoaderSettings *settings, QObject *parent);
    void removeRecursive(QLoaderSettings *settings);
    void saveItem(const QLoaderSettingsData &item, QTextStream &out);
    void saveRecursive(QLoaderSettings *settings, QTextStream &out);
    void setProperties(const QLoaderSettingsData &item, QObject *object);

public:
    QLoaderTree *const q_ptr;
    QFile *file{};
    QLoaderTree::Status status{};
    QString errorMessage;
    int errorLine{-1};
    QObject *errorObject{};
    QByteArray execLine;
    bool loaded{};
    bool modified{};
    QString infoMessage;
    QObject *infoObject{};
    bool infoChanged{};
    QString warningMessage;
    QObject *warningObject{};
    bool warningChanged{};

    struct
    {
        QHash<QStringList, QLoaderSettings*> settings;
        QHash<QLoaderSettings*, QLoaderSettingsData> data;

    } hash;

    QLoaderTreePrivate(const QString &fileName, QLoaderTree *q);
    virtual ~QLoaderTreePrivate();

    QObject *builtin(QLoaderSettings *settings, QObject *parent);
    QObject *external(QLoaderSettings *settings, QObject *parent);
    void dump(QLoaderSettings *settings) const;
    QVariant fromString(const QString &value) const;
    QString fromVariant(const QVariant &variant) const;
    bool copyOrMove(const QStringList &section, const QStringList &to, Action action);
    bool load();
    bool load(const QStringList &section);
    bool save();
};

#endif // QLOADERTREE_P_H
