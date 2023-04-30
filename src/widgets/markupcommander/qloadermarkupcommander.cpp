// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadermarkupcommander.h"

class QLoaderMarkupCommanderPrivate
{
public:
    QLoaderMarkupCommander *const q_ptr;
};

QLoaderMarkupCommander::QLoaderMarkupCommander(QLoaderSettings *settings, QWidget *parent)
:   QWidget(parent),
    QLoaderSettings(this, settings),
    d_ptr(new QLoaderMarkupCommanderPrivate{this})
{ }

QLoaderMarkupCommander::~QLoaderMarkupCommander()
{ }
