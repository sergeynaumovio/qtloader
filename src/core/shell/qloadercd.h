// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERCD_H
#define QLOADERCD_H

#include "qloadersettings.h"
#include "qloadercommandinterface.h"

class QLoaderCdPrivate;
class QLoaderShell;

class QLoaderCd : public QObject, public QLoaderSettings,
                                  public QLoaderCommandInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderCommandInterface)

    const QScopedPointer<QLoaderCdPrivate> d_ptr;

public:
    Q_INVOKABLE QLoaderCd(QLoaderSettings *settings, QLoaderShell *parent);
    ~QLoaderCd();

    QLoaderError exec(const QStringList &arguments) override;
    QString name() const override;
    QStringList tab(const QStringList &arguments) override;
};

#endif // QLOADERCD_H
