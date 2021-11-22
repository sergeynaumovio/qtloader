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

#ifndef QLOADERTREE_P_H
#define QLOADERTREE_P_H

#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QLoaderTree>

class QLoaderSettings;
class QLoaderTree;
class QFile;

struct QLoaderSettingsData
{
    QLoaderSettings *parent;
    QStringList section;
    std::string className;
    QHash<QString, QVariant> properties;
    std::vector<QLoaderSettings*> children;
};

struct QLoaderTreePrivate
{
    QLoaderTree *q_ptr;
    QLoaderTree::Status status{};
    QFile *file;
    int fileLineNumber{};
    bool isLoaded{};
    QHash<QLoaderSettings*, QLoaderSettingsData> hash;

    QLoaderTreePrivate(const QString &fileName, QLoaderTree *q);
    ~QLoaderTreePrivate();

    QObject *builtin(QLoaderSettings *objectSettings, QObject *parent);
    QObject *external(QLoaderSettings *objectSettings, QObject *parent);
    bool load();
};

#endif // QLOADERTREE_P_H
