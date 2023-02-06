// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERMOVABLEINTERFACE_H
#define QLOADERMOVABLEINTERFACE_H

#include <QObject>

class QLoaderMovableInterface
{
public:
    virtual bool isMovable(const QStringList &to) const = 0;
};
Q_DECLARE_INTERFACE(QLoaderMovableInterface, "QLoaderMovableInterface")

#endif // QLOADERMOVABLEINTERFACE_H
