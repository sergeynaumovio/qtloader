// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERTERMINALINTERFACE_H
#define QLOADERTERMINALINTERFACE_H

#include <QObject>

class QPlainTextEdit;

class QLoaderTerminalInterface
{
public:
    virtual QPlainTextEdit *out() = 0;
    virtual void setCurrentSection(const QStringList &section) = 0;
};
Q_DECLARE_INTERFACE(QLoaderTerminalInterface, "QLoaderTerminalInterface")

#endif // QLOADERTERMINALINTERFACE_H
