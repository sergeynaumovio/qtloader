// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERCOMMANDINTERFACE_H
#define QLOADERCOMMANDINTERFACE_H

#include "qloadererror.h"
#include <QObject>

class QLoaderCommandInterface
{
public:
    virtual QLoaderError exec(const QStringList &arguments) = 0;
    virtual QString name() const = 0;
    virtual QStringList tab(const QStringList &arguments) = 0;
};
Q_DECLARE_INTERFACE(QLoaderCommandInterface, "QLoaderCommandInterface")

#endif // QLOADERCOMMANDINTERFACE_H
