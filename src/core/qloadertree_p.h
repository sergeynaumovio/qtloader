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
#include <QMutex>
#include <QUuid>

class QLoaderSettings;
class QLoaderTree;
class QFile;
class QTextStream;
class QLoaderTreePrivateData;
template<typename Interface> class QLoaderTreeSection;
class QLoaderCopyInterface;
class QLoaderMoveInterface;
class QLoaderData;
class QLoaderStoragePrivate;
class QLoaderBlob;

struct QLoaderProperty
{
    bool isBlob{};
    bool isValue{};
    QString string;
    void operator =(const QString &s) { string = s; }
    operator const QString &() { return string; }
};

struct QLoaderSettingsData
{
    QLoaderSettings *parent{};
    QStringList section;
    int sectionLine{};
    QByteArray className;
    QObject *object{};
    QLoaderSettings *settings{};
    QMap<QString, QLoaderProperty> properties;
    std::vector<QLoaderSettings*> children;

    void clear();
};

class QLoaderTreePrivate
{
    QLoaderTreePrivateData &d;
    std::aligned_storage_t<240, sizeof (ptrdiff_t)> d_storage;
    void copyRecursive(QLoaderSettings *settings,
                       const QLoaderTreeSection<QLoaderCopyInterface> &src,
                       const QLoaderTreeSection<QLoaderCopyInterface> &dst);
    void dumpRecursive(QLoaderSettings *settings) const;
    QLoaderTree::Error load(const QStringList &section);
    QLoaderTree::Error loadRecursive(QLoaderSettings *settings, QObject *parent);
    void moveRecursive(QLoaderSettings *settings,
                       const QLoaderTreeSection<QLoaderMoveInterface> &src,
                       const QLoaderTreeSection<QLoaderMoveInterface> &dst);
    QLoaderTree::Error seekBlobs();
    QLoaderTree::Error readSettings();
    void removeRecursive(QLoaderSettings *settings);
    void saveItem(const QLoaderSettingsData &item, QTextStream &out);
    void saveRecursive(QLoaderSettings *settings, QTextStream &out);
    void setProperties(const QLoaderSettingsData &item, QObject *object);

public:
    QLoaderTree *const q_ptr;
    QFile *file{};
    QMutex mutex;
    bool loaded{};
    bool modified{};

    std::optional<QString> errorMessage;
    std::optional<QString> infoMessage;
    std::optional<QString> warningMessage;

    struct
    {
        QHash<QStringList, QLoaderSettings*> settings;
        QHash<QLoaderSettings*, QLoaderSettingsData> data;
        QHash<QUuid, qint64> blobs;

    } hash;

    QLoaderTreePrivate(const QString &fileName, QLoaderTree *q);
    virtual ~QLoaderTreePrivate();

    QLoaderBlob blob(const QUuid &uuid) const;
    QObject *builtin(QLoaderSettings *settings, QObject *parent);
    QLoaderTree::Error copy(const QStringList &section, const QStringList &to);
    QUuid createStorageUuid() const;
    QLoaderData *data() const;
    void dump(QLoaderSettings *settings) const;
    void emitSettingsChanged();
    QObject *external(QLoaderTree::Error &error, QLoaderSettings *settings, QObject *parent);
    QVariant fromString(const QString &value) const;
    QString fromVariant(const QVariant &variant) const;
    bool isSaving() const;
    QLoaderTree::Error load();
    QLoaderTree::Error move(const QStringList &section, const QStringList &to);
    bool removeBlob(const QUuid &id);
    QLoaderTree::Error save();
    void setStorageData(QLoaderStoragePrivate &d);
    QLoaderShell *shell() const;
};

#endif // QLOADERTREE_P_H
