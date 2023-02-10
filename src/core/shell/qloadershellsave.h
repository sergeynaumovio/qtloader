// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSHELLSAVE_H
#define QLOADERSHELLSAVE_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderShell;

class QLoaderShellSave : public QObject, public QLoaderSettings,
                                         public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    QLoaderShell *const shell;

public:
    Q_INVOKABLE QLoaderShellSave(QLoaderSettings *settings, QLoaderShell *parent);

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADERSHELLSAVE_H
