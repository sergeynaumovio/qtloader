// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADEREXIT_H
#define QLOADEREXIT_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderShell;

class QLoaderExit : public QObject, public QLoaderSettings,
                                    public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    QLoaderShell *const shell;

public:
    Q_INVOKABLE QLoaderExit(QLoaderSettings *settings, QLoaderShell *parent);

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADEREXIT_H
