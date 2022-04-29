/****************************************************************************
**
** Copyright (C) 2021, 2022 Sergey Naumov
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

#ifndef QLOADERTREE_H
#define QLOADERTREE_H

#include "qtloaderglobal.h"
#include "qloadererror.h"
#include <QObject>

class QLoaderTreePrivate;
class QLoaderData;
class QLoaderShell;

class Q_LOADER_EXPORT QLoaderTree : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QLoaderTree)

protected:
    const QScopedPointer<QLoaderTreePrivate> d_ptr;

    QLoaderTree(QLoaderTreePrivate &d, QObject *parent = nullptr);

Q_SIGNALS:
    void errorChanged(QObject *sender, QString message);
    void infoChanged(QObject *sender, QString message);
    void loaded();
    void settingsChanged();
    void warningChanged(QObject *sender, QString message);

public:
    explicit QLoaderTree(const QString &fileName, QObject *parent = nullptr);
    ~QLoaderTree();

    QLoaderError backup();
    bool contains(const QStringList &section) const;
    QLoaderError copy(const QStringList &section, const QStringList &to);
    QLoaderShell *createShell() const;
    QLoaderData *data() const;
    QString fileName() const;
    bool isModified() const;
    QLoaderError load() const;
    QLoaderError move(const QStringList &section, const QStringList &to);
    QObject *object(const QStringList &section) const;
    QLoaderError save() const;
};

#endif // QLOADERTREE_H
