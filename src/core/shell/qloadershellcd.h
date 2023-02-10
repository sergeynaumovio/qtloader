// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSHELLCD_H
#define QLOADERSHELLCD_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderShellCdPrivate;
class QLoaderShell;

class QLoaderShellCd : public QObject, public QLoaderSettings,
                                       public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    const QScopedPointer<QLoaderShellCdPrivate> d_ptr;

public:
    Q_INVOKABLE QLoaderShellCd(QLoaderSettings *settings, QLoaderShell *parent);
    ~QLoaderShellCd();

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADERSHELLCD_H
