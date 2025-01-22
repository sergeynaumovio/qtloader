// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERMARKUPCOMMANDER_H
#define QLOADERMARKUPCOMMANDER_H

#include "qloadersettings.h"
#include <QWidget>

class QLoaderMarkupCommanderPrivate;

class Q_LOADER_EXPORT QLoaderMarkupCommander : public QWidget, public QLoaderSettings
{
    Q_OBJECT

    const QScopedPointer<QLoaderMarkupCommanderPrivate> d_ptr;

public:
    QLoaderMarkupCommander(QLoaderSettings *settings, QWidget *parent);
    ~QLoaderMarkupCommander();
};

#endif // QLOADERMARKUPCOMMANDER_H
