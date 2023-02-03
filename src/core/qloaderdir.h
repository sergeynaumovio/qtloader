// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERDIR_H
#define QLOADERDIR_H

#include "qloadersettings.h"
#include "qloadericoninterface.h"

class QLoaderData;

class Q_LOADER_EXPORT QLoaderDir : public QObject, public QLoaderSettings,
                                                          QLoaderIconInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderIconInterface)

public:
    QLoaderDir(QLoaderSettings *settings, QLoaderData *parent);
    QLoaderDir(QLoaderSettings *settings, QLoaderDir *parent);

    QIcon icon() const override;
};

#endif // QLOADERDIR_H
