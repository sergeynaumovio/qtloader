// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERTERMINAL_H
#define QLOADERTERMINAL_H

#include "qloadersettings.h"
#include <QPlainTextEdit>

class QLoaderShell;
class QLoaderTerminalPrivate;

class Q_LOADER_EXPORT QLoaderTerminal : public QPlainTextEdit, public QLoaderSettings
{
    Q_OBJECT

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

    void clear();
    QLoaderShell *shell() const;
};

#endif // QLOADERTERMINAL_H
