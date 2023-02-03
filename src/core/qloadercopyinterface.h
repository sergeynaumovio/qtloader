// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERCOPYINTERFACE_H
#define QLOADERCOPYINTERFACE_H

#include <QObject>

class QLoaderCopyInterface
{
public:
    virtual bool copy(const QStringList &to) const = 0;
    virtual void copy(QObject *from) = 0;
};
Q_DECLARE_INTERFACE(QLoaderCopyInterface, "QLoaderCopyInterface")

#endif // QLOADERCOPYINTERFACE_H
