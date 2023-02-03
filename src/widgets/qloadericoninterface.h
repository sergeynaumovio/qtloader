// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERICONINTERFACE_H
#define QLOADERICONINTERFACE_H

#include <QIcon>

class QLoaderIconInterface
{
public:
    virtual QIcon icon() const = 0;
};
Q_DECLARE_INTERFACE(QLoaderIconInterface, "QLoaderIconInterface")

#endif // QLOADERICONINTERFACE_H
