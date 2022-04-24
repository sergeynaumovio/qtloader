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

#include "qloaderterminal.h"
#include "qloadertree.h"
#include <QKeyEvent>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QStack>
#include <QTextBlock>
#include <QTextCursor>

static int position(const QTextCursor &textCursor)
{
    return textCursor.position() - textCursor.block().position();
}

class QLoaderTerminalPrivate
{
public:
    QLoaderTerminal *const q_ptr;

    QString path;
    QTextCursor cursor;

    struct
    {
        QStack<QString> up;
        QStack<QString> down;
        bool skip{};

    } history;

    struct
    {
        bool onFocus{};
        bool onLeave{};

    } updateViewport;

    void clearLine()
    {
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        q_ptr->insertPlainText(path);
    }

    void keyDown()
    {
        if (history.down.size() && history.skip)
        {
            history.up.push(history.down.pop());
            history.skip = false;
        }

        if (history.down.size())
        {
            QString string = history.down.pop();
            history.up.push(string);

            clearLine();
            q_ptr->insertPlainText(string);
        }
        else
            clearLine();
    }

    void keyHome()
    {
        cursor = q_ptr->textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, path.length());
        q_ptr->setTextCursor(cursor);
    }

    bool keyLeft()
    {
        if (position(q_ptr->textCursor()) > path.size())
        {
            cursor.movePosition(QTextCursor::Left);

            return true;
        }

        return false;
    }

    void keyReturn()
    {
        cursor.select(QTextCursor::LineUnderCursor);
        QString string = cursor.selectedText();
        string.remove(0, path.length());

        if (string.length() > 0)
        {
            while (history.down.count() > 0)
                history.up.push(history.down.pop());

            history.up.push(string);
        }

        q_ptr->moveCursor(QTextCursor::EndOfLine);

        if (string.length() > 0)
        {
            q_ptr->setFocus();
            q_ptr->insertPlainText("\n");

            [&]() // exec command
            {
                q_ptr->insertPlainText(string);
                q_ptr->insertPlainText("\n");
                q_ptr->insertPlainText(path);
            }();

            cursor = q_ptr->textCursor();
        }
        else
        {
            q_ptr->insertPlainText("\n");
            q_ptr->insertPlainText(path);
        }

        q_ptr->ensureCursorVisible();
    }

    void keyUp()
    {
        if (history.up.count() > 0)
        {
            QString string = history.up.pop();
            history.down.push(string);

            clearLine();
            q_ptr->insertPlainText(string);
        }

        history.skip = true;
    }

    void setPath(const QStringList &section)
    {
        if (!q_ptr->tree()->contains(section))
        {
            q_ptr->emitError("section \"" + section.join('/') + "\" not found");
            return;
        }

        path = "[" + section.join('/') + "] ";
    }

    QLoaderTerminalPrivate(QLoaderTerminal *q)
    :   q_ptr(q)
    {
        setPath(q->value("home", q->section()).toStringList());
    }
};

void QLoaderTerminal::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;
    menu.addAction("&Copy")->setShortcut(QKeySequence("Ctrl+Shift+C"));
    menu.addAction("&Paste")->setShortcut(QKeySequence("Ctrl+Shift+V"));
    menu.addSeparator();
    menu.addAction("Select &All")->setShortcut(QKeySequence("Ctrl+Shift+A"));
    menu.exec(e->globalPos());
}

void QLoaderTerminal::keyPressEvent(QKeyEvent *e)
{
    bool control{};
    bool alt{};
    bool shift{};

    if (e->modifiers() & Qt::ControlModifier)
        control = true;

    if (e->modifiers() & Qt::AltModifier)
        alt = true;

    if (e->modifiers() & Qt::ShiftModifier)
        shift = true;

    bool modifier = control || alt || shift;

    if (control && shift && e->key() == Qt::Key_C)
    {
        copy();

        return;
    }

    auto setTextEditorInteraction = [&]
    {
        QTextCursor cursor = textCursor();
        cursor.setPosition(d_ptr->cursor.position());
        setTextCursor(cursor);
    };

    if (!modifier && textInteractionFlags().testFlag(Qt::TextBrowserInteraction))
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
        setTextEditorInteraction();
    }

    if (e->key() == Qt::Key_Backspace)
    {
        if (d_ptr->keyLeft())
            QPlainTextEdit::keyPressEvent(e);

        return;
    }

    if (e->key() == Qt::Key_Down)
    {
        if (modifier)
            setTextEditorInteraction();

        d_ptr->keyDown();

        return;
    }

    if (e->key() == Qt::Key_End)
    {
        if (modifier)
        {
            setTextEditorInteraction();

            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::EndOfBlock);
            d_ptr->cursor = cursor;
            setTextCursor(cursor);
        }
        else
            QPlainTextEdit::keyPressEvent(e);

        d_ptr->cursor = textCursor();

        return;
    }

    if (e->key() == Qt::Key_Home)
    {
        if (modifier)
            setTextEditorInteraction();

        d_ptr->keyHome();

        return;
    }

    if (e->key() == Qt::Key_Left)
    {
        if (modifier)
        {
            setTextEditorInteraction();
            d_ptr->keyHome();
        }
        else if (d_ptr->keyLeft())
            QPlainTextEdit::keyPressEvent(e);

        return;
    }

    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if (modifier)
            setTextEditorInteraction();

        d_ptr->keyReturn();

        return;
    }

    if (e->key() == Qt::Key_Right)
    {
        if (modifier)
        {
            setTextEditorInteraction();
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::EndOfBlock);
            d_ptr->cursor = cursor;
            setTextCursor(cursor);
        }
        else
        {
            QPlainTextEdit::keyPressEvent(e);
            d_ptr->cursor = textCursor();
        }

        return;
    }

    if (e->key() == Qt::Key_Up)
    {
        if (modifier)
            setTextEditorInteraction();

        d_ptr->keyUp();

        return;
    }

    QPlainTextEdit::keyPressEvent(e);
}

void QLoaderTerminal::mousePressEvent(QMouseEvent *e)
{
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    QPlainTextEdit::mousePressEvent(e);
}

void QLoaderTerminal::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent(e);
    QTextCursor cursor = d_ptr->cursor;
    QString string = cursor.block().text();
    QRect rect = cursorRect(cursor);
    rect.setWidth(rect.width() - 1);
    rect.setHeight(rect.height() - 1);
    QPainter painter(viewport());

    if (hasFocus())
    {
        if (d_ptr->updateViewport.onFocus)
        {
            viewport()->update();
            d_ptr->updateViewport.onFocus = false;
        }

        painter.fillRect(cursorRect(cursor), Qt::white);
        painter.setPen(Qt::white);

        if (position(cursor) >= 0 && position(cursor) < string.size())
        {
            painter.setPen(Qt::black);
            QChar chr = cursor.block().text().at(position(cursor));
            painter.drawText(rect, chr);
        }

        d_ptr->updateViewport.onLeave = true;
    }
    else
    {
        if (d_ptr->updateViewport.onLeave)
        {
            viewport()->update();
            d_ptr->updateViewport.onLeave = false;
        }

        painter.fillRect(cursorRect(cursor), Qt::black);
        painter.setPen(Qt::white);
        painter.drawRect(rect);

        QChar chr = ' ';
        if (position(cursor) >= 0 && position(cursor) < string.size())
            chr = cursor.block().text().at(position(cursor));
        painter.drawText(rect, chr);

        d_ptr->updateViewport.onFocus = true;
    }
}

QLoaderTerminal::QLoaderTerminal(QLoaderSettings *settings, QWidget *parent)
:   QPlainTextEdit(parent),
    QLoaderSettings(settings),
    d_ptr(new QLoaderTerminalPrivate(this))
{
    QPalette p = palette();
    p.setColor(QPalette::Active, QPalette::Base, Qt::black);
    p.setColor(QPalette::Inactive, QPalette::Base, Qt::black);
    p.setColor(QPalette::Active, QPalette::Text, Qt::white);
    p.setColor(QPalette::Inactive, QPalette::Text, Qt::white);
    p.setColor(QPalette::Highlight, QColor(Qt::white));
    p.setColor(QPalette::HighlightedText, QColor(Qt::black));
    setPalette(p);

    QFont f;
#ifdef Q_OS_LINUX
    f.setFamily("Monospace");
    f.setPointSize(11);
#else
    f.setFamily("Lucida Console");
    f.setPointSize(10);
#endif
    f.setFixedPitch(true);
    setFont(f);

    setCursorWidth(QFontMetrics(font()).boundingRect(QChar('o')).width());
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setUndoRedoEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWordWrapMode(QTextOption::WrapAnywhere);

    insertPlainText(d_ptr->path);

    connect(this, &QPlainTextEdit::textChanged, [this]
    {
        QTextCursor cursor = textCursor();
        d_ptr->cursor = cursor;
    });

    emit textChanged();

    if (parent)
        parent->layout()->addWidget(this);
    else
         show();

    setFocus();
}

QLoaderTerminal::~QLoaderTerminal()
{ }
