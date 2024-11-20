// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloaderterminal.h"
#include "qloadertree.h"
#include "qloadershell.h"
#include <QKeyEvent>
#include <QLayout>
#include <QMenu>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QStack>
#include <QTextBlock>
#include <QTextCursor>
#include <QThread>

using namespace Qt::Literals::StringLiterals;

class QLoaderTerminalHistory
{
    QLoaderTerminal *const q_ptr;

    QStack<QString> up;
    QStack<QString> down;
    bool skip{};

public:
    QLoaderTerminalHistory(QLoaderTerminal *q)
    :   q_ptr(q)
    {
        Q_UNUSED(q_ptr)
        Q_UNUSED(skip)
    }
};

class QLoaderTerminalPromt
{

};

static int position(const QTextCursor &textCursor)
{
    return textCursor.position() - textCursor.block().position();
}

class QLoaderTerminalPrivate
{
public:
    QLoaderTerminal *const q_ptr;
    QTextCursor cursor;


    QLoaderShell *shell;
    QThread thread;

    struct
    {
        QStringList section;
        QString string;

    } path;

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

    struct
    {
        QRegularExpression regex{u"[^\\s]+"_s};
        QString string;

    } command;

    QLoaderTerminalPrivate(QLoaderTerminal *q)
    :   q_ptr(q),
        shell(q->tree()->newShellInstance())
    {
        shell->setTerminal(q);
        setPath(shell->section());
        shell->moveToThread(&thread);
        thread.start();

        QObject::connect(shell, &QObject::destroyed, q, [=, this]
        {
            thread.quit();
            thread.wait();
            shell = nullptr;
            q->deleteLater();
        });
    }

    ~QLoaderTerminalPrivate()
    {
        if (shell) delete shell;
    }

    void clearLine()
    {
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.removeSelectedText();
        q_ptr->insertPlainText(path.string);
    }

    QString currentCommandLine() const
    {
        QString string = cursor.block().text();
        string.remove(0, path.string.size());
        return string;
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
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, path.string.size());
        q_ptr->setTextCursor(cursor);
    }

    bool keyLeft()
    {
        if (position(q_ptr->textCursor()) > path.string.size())
        {
            cursor.movePosition(QTextCursor::Left);

            return true;
        }

        return false;
    }

    void keyReturn()
    {
        QString string = currentCommandLine();
        if (string.size())
        {
            while (history.down.size())
                history.up.push(history.down.pop());

            history.up.push(string);
        }

        q_ptr->moveCursor(QTextCursor::EndOfLine);

        if (string.size())
        {
            QStringList pipeline = string.split(u'|');
            QString cmd = pipeline.first();

            QRegularExpressionMatch match;
            if ((match = command.regex.match(cmd)).hasMatch())
            {
                command.string = match.captured();
                shell->exec(command.string, QProcess::splitCommand(cmd));
            }
        }

        if (!q_ptr->document()->isEmpty())
            q_ptr->insertPlainText(u"\n"_s);

        q_ptr->insertPlainText(path.string);
        q_ptr->ensureCursorVisible();
        cursor = q_ptr->textCursor();
    }

    void keyUp()
    {
        if (history.up.size())
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
        if (!section.isEmpty() && !q_ptr->tree()->contains(section))
        {
            q_ptr->emitError(u"section \""_s + section.join(u'/') + u"\" not found"_s);
            return;
        }

        if (section.isEmpty())
            path.section = q_ptr->section();
        else if (q_ptr->tree()->contains(section))
            path.section = section;

        path.string = u'[' + path.section.join(u'/') + u"] "_s;
    }
};

void QLoaderTerminal::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu menu;
    menu.addAction(u"&Copy"_s)->setShortcut(QKeySequence(u"Ctrl+Shift+C"_s));
    menu.addAction(u"&Paste"_s)->setShortcut(QKeySequence(u"Ctrl+Shift+V"_s));
    menu.addSeparator();
    menu.addAction(u"Select &All"_s)->setShortcut(QKeySequence(u"Ctrl+Shift+A"_s));
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

    if (e->key() == Qt::Key_Tab)
        return;

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

        QChar chr(u' ');
        if (position(cursor) >= 0 && position(cursor) < string.size())
            chr = cursor.block().text().at(position(cursor));
        painter.drawText(rect, chr);

        d_ptr->updateViewport.onFocus = true;
    }
}

QLoaderTerminal::QLoaderTerminal(QLoaderSettings *settings, QWidget *parent)
:   QPlainTextEdit(parent),
    QLoaderSettings(this, settings),
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
    f.setFamily(u"Monospace"_s);
    f.setPointSize(11);
#else
    f.setFamily(u"Lucida Console"_s);
    f.setPointSize(10);
#endif
    f.setFixedPitch(true);
    setFont(f);

    setCursorWidth(QFontMetrics(font()).boundingRect(QChar(u'o')).width());
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setUndoRedoEnabled(false);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setWordWrapMode(QTextOption::WrapAnywhere);

    insertPlainText(d_ptr->path.string);

    connect(this, &QPlainTextEdit::textChanged, [this]{ d_ptr->cursor = textCursor(); });

    emit textChanged();

    if (parent)
        parent->layout()->addWidget(this);

    setFocus();
}

QLoaderTerminal::~QLoaderTerminal()
{ }

QPlainTextEdit *QLoaderTerminal::out()
{
    return this;
}

void QLoaderTerminal::setCurrentSection(const QStringList &section)
{
    d_ptr->setPath(section);
}
