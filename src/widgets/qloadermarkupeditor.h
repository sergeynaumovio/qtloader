// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#ifndef QLOADERMARKUPEDITOR_H
#define QLOADERMARKUPEDITOR_H

#include "qloadersettings.h"
#include <QTextEdit>

class QLoaderMarkupEditorPrivate;

class Q_LOADER_EXPORT QLoaderMarkupEditor : public QTextEdit, public QLoaderSettings
{
    Q_OBJECT

    friend class QLoaderMarkupEditorPrivate;
    const QScopedPointer<QLoaderMarkupEditorPrivate> d_ptr;

public:
    QLoaderMarkupEditor(QLoaderSettings *settings, QWidget *parent = nullptr);
    ~QLoaderMarkupEditor();
};

#endif // QLOADERMARKUPEDITOR_H
