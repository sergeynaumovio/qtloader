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

#include "qloadertree_p.h"
#include <QFile>
#include <QPluginLoader>
#include <QLoaderTree>
#include <QLoaderSettings>
#include <QLoaderInterface>

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   q_ptr(q)
{
    if (!QFile::exists(fileName))
    {
        status = QLoaderTree::AccessError;
        return;
    }

    file = new QFile(fileName, q);

    if (!file->open(QIODevice::ReadWrite | QIODevice::Text))
    {
        status = QLoaderTree::AccessError;
        return;
    }

    QLoaderSettings *objectSettings{};
    errorLine = 0;
    while (!file->atEnd())
    {
        QByteArray line = file->readLine();
        ++errorLine;

        QRegularExpression comment("^#.*");
        if (comment.match(line).hasMatch())
            continue;

        QRegularExpression section("^\\[[^\\[\\]]*\\]$");
        if (section.match(line).hasMatch())
        {
            objectSettings = new QLoaderSettings(this);

            QRegularExpression sectionGroups("^[\\/\\[\\]]+");
            QLoaderSettingsData data;
            data.section = sectionGroups.match(line).capturedTexts();
            hash[objectSettings] = data;

            continue;
        }

        QRegularExpression keyValue("^(?<key>[^=]*[\\w]+)\\s*=\\s*(?<value>.+)$");
        QRegularExpressionMatch keyValueMatch = keyValue.match(line);
        if (keyValueMatch.hasMatch())
        {
            QString key = keyValueMatch.captured("key");
            QString value = keyValueMatch.captured("value");

            if (key == "class")
            {
                if (!objectSettings)
                {
                    status = QLoaderTree::FormatError;
                    return;
                }

                hash[objectSettings].className = value.toStdString();
                break;
            }
        }
    }
    errorLine = -1;
}

QLoaderTreePrivate::~QLoaderTreePrivate()
{
    QHashIterator<QLoaderSettings*, QLoaderSettingsData> i(hash);
    while (i.hasNext())
    {
        i.next();
        delete i.key();
    }

    hash.clear();
}

QObject *QLoaderTreePrivate::builtin(QLoaderSettings *settings, QObject */*parent*/)
{
    QString className = settings->className();

    if (className.startsWith("Loader")) { }

    return nullptr;
}

QObject *QLoaderTreePrivate::external(QLoaderSettings *objectSettings, QObject */*parent*/)
{
    const char *className = objectSettings->className();
    QString libraryName("Qt" + QString::number(QT_VERSION_MAJOR));

    QRegularExpression starts("^[A-Z]+[a-z,0-9]*");
    QRegularExpressionMatch startsMatch = starts.match(className);
    if (startsMatch.hasMatch())
    {
        libraryName += startsMatch.captured();
        QPluginLoader loader(libraryName);
        if (!loader.instance())
        {
            status = QLoaderTree::PluginError;
            return nullptr;
        }

        QLoaderInterface *plugin = qobject_cast<QLoaderInterface*>(loader.instance());
        if (!plugin)
        {
            status = QLoaderTree::PluginError;
            return nullptr;
        }

        return plugin->object(objectSettings, nullptr);
    }

    return nullptr;
}

bool QLoaderTreePrivate::load()
{
    if (isLoaded)
        return false;

    QHashIterator<QLoaderSettings*, QLoaderSettingsData> i(hash);
    while (i.hasNext())
    {
        i.next();
        QObject *object = external(i.key(), nullptr);

        if (!object && status != QLoaderTree::PluginError)
            status = QLoaderTree::ObjectError;

        if (!i.value().parent)
            break;
    }

    isLoaded = true;

    return !static_cast<unsigned char>(status);
}
