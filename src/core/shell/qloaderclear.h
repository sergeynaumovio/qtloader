// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERCLEAR_H
#define QLOADERCLEAR_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderShell;

class QLoaderClear : public QObject, public QLoaderSettings,
                                     public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    QLoaderShell *const shell;

public:
    Q_INVOKABLE QLoaderClear(QLoaderSettings *settings, QLoaderShell *parent);

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADERCLEAR_H
