// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERSAVEINTERFACE_H
#define QLOADERSAVEINTERFACE_H

#include <QObject>

class QLoaderSaveInterface
{
public:
    virtual void save() = 0;
};
Q_DECLARE_INTERFACE(QLoaderSaveInterface, "QLoaderSaveInterface")

#endif // QLOADERSAVEINTERFACE_H
