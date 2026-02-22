// Copyright (C) 2026 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadermarkupeditor.h"
#include "qloadertree.h"
#include <QFile>
#include <QList>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextStream>

using namespace Qt::Literals::StringLiterals;

class Highlighter : public QSyntaxHighlighter
{
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightingRule> highlightingRules;

    const QRegularExpression shebang{u"^\\#!.+"_s};
    const QRegularExpression comment{u"^\\#[^!]*$"_s};
    const QRegularExpression sectionName{u"^\\[([^\\[\\]]*)\\]$"_s};
    const QRegularExpression keyValue{u"^(\\w+)[^=\\r\\n\\[\\]](?:\\ *=\\ *)([^\\n]*)"_s};

    QTextCharFormat shebangFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat sectionFormat;
    QTextCharFormat keyFormat;
    QTextCharFormat valueFormat;

protected:
    void highlightBlock(const QString &text) override
    {
        QRegularExpressionMatch match;

        for (const HighlightingRule &rule : std::as_const(highlightingRules))
        {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext())
            {
                match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        if ((match = keyValue.match(text)).hasMatch())
        {
            setFormat(match.capturedStart(1), match.capturedLength(1), keyFormat);
            setFormat(match.capturedStart(2), match.capturedLength(2), valueFormat);
        }
    }

public:
    Highlighter(QTextDocument *parent)
    :   QSyntaxHighlighter(parent)
    {
        HighlightingRule rule;

        shebangFormat.setForeground(Qt::darkBlue);
        rule.pattern = shebang;
        rule.format = shebangFormat;
        highlightingRules.append(rule);

        commentFormat.setForeground(Qt::darkGreen);
        rule.pattern = comment;
        rule.format = commentFormat;
        highlightingRules.append(rule);

        sectionFormat.setFontWeight(QFont::Bold);
        sectionFormat.setForeground(Qt::darkMagenta);
        rule.pattern = sectionName;
        rule.format = sectionFormat;
        highlightingRules.append(rule);

        keyFormat.setForeground(Qt::darkRed);
        valueFormat.setForeground(Qt::darkGreen);
    }
};

class QLoaderMarkupEditorPrivate
{
    Highlighter *const highlighter;

public:
    QLoaderMarkupEditorPrivate(QLoaderMarkupEditor *q)
    :   highlighter(new Highlighter(q->document()))
    {
        QFile file(q->tree()->fileName());

        if (file.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream stream(&file);
            q->setPlainText(stream.readAll());
        }
    }
};

QLoaderMarkupEditor::QLoaderMarkupEditor(QLoaderSettings *settings, QWidget *parent)
:   QTextEdit(parent),
    QLoaderSettings(this, settings),
    d_ptr(new QLoaderMarkupEditorPrivate(this))
{
    QFont font;
    font.setFamily(u"Courier"_s);
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);

    if (!parent)
        setAttribute(Qt::WA_DeleteOnClose);
}

QLoaderMarkupEditor::~QLoaderMarkupEditor()
{ }
