// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERTERMINAL_H
#define QLOADERTERMINAL_H

#include "qloadersettings.h"
#include "qloaderterminalinterface.h"
#include <QPlainTextEdit>

class QLoaderTerminalPrivate;

class Q_LOADER_EXPORT QLoaderTerminal : public QPlainTextEdit, public QLoaderSettings,
                                                               public QLoaderTerminalInterface
{
    Q_OBJECT
    Q_INTERFACES(QLoaderTerminalInterface)

    friend class QLoaderTerminalPrivate;
    const QScopedPointer<QLoaderTerminalPrivate> d_ptr;

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent *e) override;

public:
    Q_INVOKABLE QLoaderTerminal(QLoaderSettings *settings, QWidget *parent = nullptr);
    ~QLoaderTerminal();

    QPlainTextEdit *out() override;
    void setCurrentSection(const QStringList &section) override;
};

#endif // QLOADERTERMINAL_H
