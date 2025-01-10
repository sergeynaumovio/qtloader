// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERPLUGININTERFACE_H
#define QLOADERPLUGININTERFACE_H

#include <QObject>

class QLoaderSettings;
class QLoaderPluginInterface
{
public:
    virtual QObject *object(QLoaderSettings *settings, QObject *parent) const = 0;
};
Q_DECLARE_INTERFACE(QLoaderPluginInterface, "QLoaderPluginInterface")

#endif // QLOADERPLUGININTERFACE_H
