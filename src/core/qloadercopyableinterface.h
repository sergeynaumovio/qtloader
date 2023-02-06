// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERCOPYABLEINTERFACE_H
#define QLOADERCOPYABLEINTERFACE_H

#include <QObject>

class QLoaderCopyableInterface
{
public:
    virtual bool isCopyable(const QStringList &to) const = 0;
};
Q_DECLARE_INTERFACE(QLoaderCopyableInterface, "QLoaderCopyableInterface")

#endif // QLOADERCOPYABLEINTERFACE_H
