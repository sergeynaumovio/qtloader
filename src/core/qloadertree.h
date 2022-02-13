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
#include <QObject>

class QLoaderTreePrivate;
class QLoaderSettings;

class Q_LOADER_EXPORT QLoaderTree : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QLoaderTree)

    Q_PROPERTY(int errorLine READ errorLine NOTIFY errorLineChanged)
    Q_PROPERTY(QString fileName READ fileName CONSTANT)
    Q_PROPERTY(bool isModified READ isModified NOTIFY settingsChanged)
    Q_PROPERTY(QLoaderTree::Status status READ status NOTIFY statusChanged)

protected:
    const QScopedPointer<QLoaderTreePrivate> d_ptr;

public:
    enum Status
    {
        NoError,
        AccessError,
        FormatError,
        DesignError,
        PluginError,
        ObjectError
    };
    Q_ENUM(Status)

Q_SIGNALS:
    void errorLineChanged(int);
    void errorChanged(QObject *, QString);
    void infoChanged(QObject *, QString);
    void loaded();
    void settingsChanged();
    void statusChanged(QLoaderTree::Status);
    void warningChanged(QObject *, QString);

public:
    explicit QLoaderTree(const QString &fileName, QObject *parent = nullptr);
    ~QLoaderTree();

    bool contains(const QStringList &section) const;
    bool copy(const QStringList &section, const QStringList &to);
    QString errorMessage() const;
    int errorLine() const;
    QObject *errorObject() const;
    QString fileName() const;
    QString infoMessage() const;
    QObject *infoObject() const;
    bool isLoaded() const;
    bool isModified() const;
    bool load() const;
    bool move(const QStringList &section, const QStringList &to);
    QObject *object(const QStringList &section) const;
    virtual bool save() const;
    const QLoaderSettings *settings(const QStringList &section) const;
    QLoaderTree::Status status() const;
    QString warningMessage() const;
    QObject *warningObject() const;
};

#endif // QLOADERTREE_H
