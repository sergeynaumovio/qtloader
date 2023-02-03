// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERMOVEINTERFACE_H
#define QLOADERMOVEINTERFACE_H

#include <QObject>

class QLoaderMoveInterface
{
public:
    virtual bool move(const QStringList &to) = 0;
};
Q_DECLARE_INTERFACE(QLoaderMoveInterface, "QLoaderMoveInterface")

#endif // QLOADERMOVEINTERFACE_H
