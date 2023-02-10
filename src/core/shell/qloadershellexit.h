// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSHELLEXIT_H
#define QLOADERSHELLEXIT_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderShell;

class QLoaderShellExit : public QObject, public QLoaderSettings,
                                         public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    QLoaderShell *const shell;

public:
    Q_INVOKABLE QLoaderShellExit(QLoaderSettings *settings, QLoaderShell *parent);

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADERSHELLEXIT_H
