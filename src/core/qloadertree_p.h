// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERTREE_P_H
#define QLOADERTREE_P_H

#include <QHash>
#include <QMap>
#include <QMutex>

class QFile;
class QLoaderError;
class QLoaderSettings;
class QLoaderTree;
class QLoaderTreePrivateData;
class QLoaderTreeSection;
class QLoaderShell;
class QTextStream;

using QLoaderProperty = QString;

struct QLoaderSettingsData
{
    QLoaderSettings *parent{};
    int sectionLine{};
    QString section;
    int level;
    QByteArray className;
    QString pluginName;
    QObject *object{};
    QList<QLoaderSettings *> settings;
    QMap<QString, QLoaderProperty> properties;
    QList<QLoaderSettings *> children;
};

class QLoaderTreePrivate
{
    QLoaderTreePrivateData &d;
    alignas(8) std::byte d_storage[184];

    void copyRecursive(QLoaderSettings *settings,
                       const QLoaderTreeSection &src,
                       const QLoaderTreeSection &dst);
    void dumpRecursive(QLoaderSettings *settings) const;
    QLoaderError load(const QStringList &section);
    QLoaderError loadRecursive(QLoaderSettings *settings, QObject *parent);
    void moveRecursive(QLoaderSettings *settings,
                       const QLoaderTreeSection &src,
                       const QLoaderTreeSection &dst);
    QLoaderError readSettings();
    void removeRecursive(QLoaderSettings *settings);
    void saveItem(const QLoaderSettingsData &item, QTextStream &out);
    void saveRecursive(QLoaderSettings *settings, QTextStream &out);

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
        struct
        {
            QHash<QString, QLoaderSettings *> sections;
            QHash<QObject *, QLoaderSettings *> objects;

        } settings;

        QHash<QLoaderSettings *, QLoaderSettingsData> data;

    } hash;

    QLoaderTreePrivate(const QString &fileName, QLoaderTree *q);
    virtual ~QLoaderTreePrivate();

    QObject *builtin(QLoaderSettings *settings, QObject *parent);
    QLoaderError copy(QStringView section, QStringView to);
    void dump(QLoaderSettings *settings) const;
    void emitSettingsChanged();
    QObject *external(QLoaderError &error, QLoaderSettings *settings, QObject *parent);
    QVariant fromString(const QString &value) const;
    QString fromVariant(const QVariant &variant) const;
    bool isSaving() const;
    QLoaderError load();
    QLoaderError move(QStringView section, QStringView to);
    QLoaderShell *newShellInstance();
    QLoaderError save();
    void setProperties(const QLoaderSettingsData &item, QObject *object);
};

#endif // QLOADERTREE_P_H
