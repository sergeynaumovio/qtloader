// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERERROR_H
#define QLOADERERROR_H

#include "qtloaderglobal.h"
#include <QtCore/qmetatype.h>

class Q_LOADER_EXPORT QLoaderError
{
    Q_GADGET

public:
    enum Status
    {
        No,
        Access,
        Format,
        Design,
        Plugin,
        Object
    };
    Q_ENUM(Status)

    int line{};
    Status status{};
    QString message{};
    operator bool() const { return status; }
};

#endif // QLOADERERROR_H
