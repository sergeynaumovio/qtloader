// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERTREE_H
#define QLOADERTREE_H

#include "qtloaderglobal.h"
#include "qloadererror.h"
#include <QObject>

class QLoaderTreePrivate;
class QLoaderSettings;
class QLoaderShell;

class Q_LOADER_EXPORT QLoaderTree : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QLoaderTree)

protected:
    const QScopedPointer<QLoaderTreePrivate> d_ptr;

    QLoaderTree(QLoaderTreePrivate &d, QObject *parent = nullptr);

Q_SIGNALS:
    void errorChanged(QObject *sender, QString message);
    void infoChanged(QObject *sender, QString message);
    void loaded();
    void settingsChanged();
    void warningChanged(QObject *sender, QString message);

public:
    explicit QLoaderTree(const QString &fileName, QObject *parent = nullptr);
    ~QLoaderTree();

    QLoaderError backup();
    bool contains(const QStringList &section) const;
    QLoaderError copy(const QStringList &section, const QStringList &to);
    QString fileName() const;
    bool isLoaded() const;
    bool isModified() const;
    QLoaderError load() const;
    QLoaderError move(const QStringList &section, const QStringList &to);
    QLoaderShell *newShellInstance() const;
    QObject *object(const QStringList &section) const;
    QLoaderError save() const;
    QLoaderSettings *settings(QObject *object) const;
};

#endif // QLOADERTREE_H
