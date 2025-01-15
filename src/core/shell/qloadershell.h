// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSHELL_H
#define QLOADERSHELL_H

#include "qloadersettings.h"
#include "qloadererror.h"
#include <QObject>

class QLoaderShellPrivate;

class QLoaderCommand
{
public:
    QString name;
    QStringList arguments;
};

using QLoaderCommandList = QList<QLoaderCommand>;

class Q_LOADER_EXPORT QLoaderShell : public QObject, public QLoaderSettings
{
    Q_OBJECT

    friend class QLoaderTreePrivate;
    QScopedPointer<QLoaderShellPrivate> d_ptr;

    QLoaderShell(QLoaderSettings *settings);

Q_SIGNALS:
    void finished(QString name, QLoaderError error);
    void started(QString name, QStringList arguments);

public:
    ~QLoaderShell();

    void addCommand(QObject *command);
    bool cd(const QString &section);
    void cdHome();
    bool cdUp();
    QLoaderError error() const;
    QLoaderError exec(QLatin1StringView command);
    QLoaderError exec(const QLoaderCommand &command);
    QLoaderError exec(const QLoaderCommandList &pipeline);
    QLoaderError exec(const QString &name, const QStringList &arguments);
    QStringList section() const;
};

#endif // QLOADERSHELL_H
