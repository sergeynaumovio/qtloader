/****************************************************************************
**
** Copyright (C) 2022 Sergey Naumov
**
** Permission to use, copy, modify, and/or distribute this
** software for any purpose with or without fee is hereby granted.
**
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
** CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
** LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
** NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
** CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**
****************************************************************************/

#ifndef QLOADERTERMINAL_H
#define QLOADERTERMINAL_H

#include "qloadersettings.h"
#include <QPlainTextEdit>

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
    Q_INVOKABLE QLoaderTerminal(QLoaderSettings *settings, QWidget *parent);
    ~QLoaderTerminal();
};

#endif // QLOADERTERMINAL_H
