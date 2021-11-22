/****************************************************************************
**
** Copyright (C) 2021 Sergey Naumov
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

#include "qloadertree.h"
#include "qloadertree_p.h"
#include <QFileInfo>

QLoaderTree::QLoaderTree(const QString &fileName, QObject *parent)
:   QObject(parent),
    d_ptr(new QLoaderTreePrivate(fileName, this))
{ }

QLoaderTree::~QLoaderTree()
{ }

int QLoaderTree::errorLine() const { return d_ptr->errorLine; }

QString QLoaderTree::fileName() const { return d_ptr->file->fileName(); }

bool QLoaderTree::load() const { return d_ptr->load(); }

QLoaderTree::Status QLoaderTree::status() const { return d_ptr->status; }
