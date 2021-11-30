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
#include "qloadertree.h"
#include "qloadersettings.h"
#include "qloaderinterface.h"
#include <QFile>
#include <QPluginLoader>
#include <QMainWindow>
#include <QMenu>
#include <QAction>

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

    QStringList section;
    QLoaderSettings *settings{};
    errorLine = 0;

    while (!file->atEnd())
    {
        QByteArray line = file->readLine();
        ++errorLine;

        QRegularExpression comment("^#.*");
        if (comment.match(line).hasMatch())
            continue;

        QRegularExpression sectionName("^\\[(?<section>[^\\[\\]]*)\\]$");
        QRegularExpressionMatch sectionNameMatch = sectionName.match(line);
        if (sectionNameMatch.hasMatch())
        {
            section = sectionNameMatch.captured("section").split('/');
            settings = new QLoaderSettings(this);
            bool isValid{};
            QLoaderSettingsData item;
            item.section = section;

            if (!root.settings && section.size() == 1 && section.back().size())
            {
                root.settings = settings;
                isValid = true;
            }
            else if (root.settings && section.size() > 1 && section.back().size() &&
                     !hash.settings.contains(section))
            {
                QStringList parent = section;
                parent.removeLast();

                if (hash.settings.contains(parent))
                {
                    isValid = true;
                    item.parent = hash.settings[parent];
                    hash.data[item.parent].children.push_back(settings);
                }
            }

            if (!isValid)
            {
                status = QLoaderTree::DesignError;
                delete settings;
                return;
            }

            hash.settings[section] = settings;
            hash.data[settings] = item;
            continue;
        }

        QRegularExpression keyValue("^(?<key>[^=]*[\\w]+)\\s*=\\s*(?<value>.+)$");
        QRegularExpressionMatch keyValueMatch = keyValue.match(line);
        if (keyValueMatch.hasMatch())
        {
            QString key = keyValueMatch.captured("key");
            QVariant value = keyValueMatch.captured("value");

            if (key == "class")
            {
                if (!settings)
                {
                    status = QLoaderTree::FormatError;
                    return;
                }

                hash.data[settings].className = value.toByteArray();
                hash.data[settings].classLine = errorLine;
            }
            else
            {
                hash.data[settings].properties[key] = value;
            }
        }
    }

    errorLine = -1;
}

QLoaderTreePrivate::~QLoaderTreePrivate()
{
    QHashIterator<QStringList, QLoaderSettings*> i(hash.settings);
    while (i.hasNext())
    {
        i.next();
        delete i.value();
    }
}

QObject *QLoaderTreePrivate::builtin(QLoaderSettings* /*settings*/, QObject* /*parent*/)
{
    return nullptr;
}

QObject *QLoaderTreePrivate::external(QLoaderSettings *settings, QObject *parent)
{
    const char *className = settings->className();
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

        return plugin->object(settings, parent);
    }

    return nullptr;
}

bool QLoaderTreePrivate::copy(const QStringList& /*from*/, const QStringList& /*to*/)
{
    return false;
}

void QLoaderTreePrivate::setProperties(QLoaderSettings *settings, QObject *object)
{
    object->setObjectName(hash.data[settings].section.last());

    settings = qobject_cast<QLoaderSettings*>(object);

    if (settings)
    {
        QVariant value;
        QAction *action = qobject_cast<QAction*>(object);
        if (action)
        {
            if (!(value = settings->value("autoRepeat")).isNull())
                action->setAutoRepeat(value.toBool());

            if (!(value = settings->value("checkable")).isNull())
                action->setCheckable(value.toBool());

            if (!(value = settings->value("checked")).isNull())
                action->setChecked(value.toBool());

            if (!(value = settings->value("enabled")).isNull())
                action->setEnabled(value.toBool());

            if (!(value = settings->value("text")).isNull())
                action->setText(value.toString());

            return;
        }

        QWidget *widget = qobject_cast<QWidget*>(object);
        if (widget)
        {
            if (!(value = settings->value("enabled")).isNull())
                widget->setEnabled(value.toBool());

            if (!(value = settings->value("minimumWidth")).isNull())
                widget->setMinimumWidth(value.toInt());

            if (!(value = settings->value("minimumHeight")).isNull())
                widget->setMinimumHeight(value.toInt());

            if (!(value = settings->value("styleSheet")).isNull())
                widget->setStyleSheet(value.toString());

            if (!(value = settings->value("visible")).isNull())
                widget->setVisible(value.toBool());
        }

        QMainWindow *mainwindow = qobject_cast<QMainWindow*>(object);
        if (mainwindow)
        {
            if (!(value = settings->value("windowTitle")).isNull())
                mainwindow->setWindowTitle(value.toString());

            return;
        }

        QMenu *menu = qobject_cast<QMenu*>(object);
        if (menu)
        {
            if (!(value = settings->value("title")).isNull())
                menu->setTitle(value.toString());

            return;
        }
    }
}

void QLoaderTreePrivate::load(QLoaderSettings *settings, QObject *parent)
{
    QObject *object;
    if (!qstrncmp(hash.data[settings].className, "Loader", 6))
        object = builtin(settings, parent);
    else
        object = external(settings, parent);

    if (!object || object == parent)
    {
        if (!object && !status)
            status = QLoaderTree::ObjectError;
        else if (object == parent)
            status = QLoaderTree::ParentError;

        errorLine = hash.data[settings].classLine;
        return;
    }

    if (!parent)
        root.object = object;

    setProperties(settings, object);

    for (QLoaderSettings *child : hash.data[settings].children)
    {
        if (!status)
            load(child, object);
    }
}

bool QLoaderTreePrivate::load()
{
    if (loaded)
        return false;

    loaded = true;

    load(root.settings, nullptr);

    if (status && root.object)
        root.object->deleteLater();

    return (status == QLoaderTree::NoError);
}

bool QLoaderTreePrivate::save()
{
    modified = false;
    return false;
}
