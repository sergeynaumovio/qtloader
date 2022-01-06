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
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QTextStream>

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   q_ptr(q),
    file(new QFile(fileName, q))
{
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        status = QLoaderTree::AccessError;
        return;
    }

    QStringList section;
    QLoaderSettings *settings{};
    errorLine = 0;
    char comment = '#';

    while (!file->atEnd())
    {
        QByteArray line = file->readLine();
        ++errorLine;

        if (errorLine == 1 && line.startsWith("#!"))
        {
            execLine = line;
            continue;
        }

        if (line.startsWith(comment))
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
    file->close();
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

bool QLoaderTreePrivate::copy(const QStringList& /*section*/, const QStringList& /*to*/)
{
    if (loaded)
    {
        modified = true;
        emit q_ptr->settingsChanged();
    }

    return false;
}

void QLoaderTreePrivate::setProperties(QLoaderSettings *settings, QObject *object)
{
    object->setObjectName(hash.data[settings].section.last());

    const QMap<QString, QVariant> &properties = hash.data[settings].properties;
    auto value = [&properties](const QString &key, const QVariant defaultValue = QVariant())
    {
        if (properties.contains(key))
            return properties[key];

        return defaultValue;
    };

    QVariant v;
    QAction *action = qobject_cast<QAction*>(object);
    if (action)
    {
        if (!(v = value("autoRepeat")).isNull())
            action->setAutoRepeat(v.toBool());

        if (!(v = value("checkable")).isNull())
            action->setCheckable(v.toBool());

        if (!(v = value("checked")).isNull())
            action->setChecked(v.toBool());

        if (!(v = value("enabled")).isNull())
            action->setEnabled(v.toBool());

        if (!(v = value("text")).isNull())
            action->setText(v.toString());

        return;
    }

    QWidget *widget = qobject_cast<QWidget*>(object);
    if (widget)
    {
        if (!(v = value("enabled")).isNull())
            widget->setEnabled(v.toBool());

        if (!(v = value("hidden")).isNull())
            widget->setHidden(v.toBool());

        if (!(v = value("minimumWidth")).isNull())
            widget->setMinimumWidth(v.toInt());

        if (!(v = value("minimumHeight")).isNull())
            widget->setMinimumHeight(v.toInt());

        if (!(v = value("styleSheet")).isNull())
            widget->setStyleSheet(v.toString());

        if (!(v = value("visible")).isNull())
            widget->setVisible(v.toBool());
    }

    QLabel *label = qobject_cast<QLabel*>(object);
    if (label)
    {
        if (!(v = value("text")).isNull())
            label->setText(v.toString());

        return;
    }

    QMainWindow *mainwindow = qobject_cast<QMainWindow*>(object);
    if (mainwindow)
    {
        if (!(v = value("windowTitle")).isNull())
            mainwindow->setWindowTitle(v.toString());

        return;
    }

    QMenu *menu = qobject_cast<QMenu*>(object);
    if (menu)
    {
        if (!(v = value("title")).isNull())
            menu->setTitle(v.toString());

        return;
    }
}

void QLoaderTreePrivate::dumpRecursive(QLoaderSettings *settings) const
{
    qDebug().noquote().nospace() << '[' << hash.data[settings].section.join('/') << ']';
    qDebug().noquote() << "class =" << hash.data[settings].className;

    QMapIterator<QString, QVariant> i(hash.data[settings].properties);
    while (i.hasNext())
    {
        i.next();
        qDebug().noquote() << i.key() << '=' << i.value().toString();
    }

    qDebug() << "";

    for (QLoaderSettings *child : hash.data[settings].children)
    {
        dumpRecursive(child);
    }
}

void QLoaderTreePrivate::dump(QLoaderSettings *settings) const
{
    if (execLine.size())
        qDebug().noquote() << execLine;

    dumpRecursive(settings);
}

void QLoaderTreePrivate::loadRecursive(QLoaderSettings *settings, QObject *parent)
{
    QObject *object;
    if (!qstrncmp(hash.data[settings].className, "Loader", 6))
        object = builtin(settings, parent);
    else
        object = external(settings, parent);

    if (!parent)
        root.object = object;

    if (!object || object == parent || status)
    {
        if (!object)
        {
            status = QLoaderTree::ObjectError;
            error = "class not found";
        }
        else if (object && object == parent)
        {
            status = QLoaderTree::ObjectError;
            error = "parent object not valid";
        }

        errorLine = hash.data[settings].classLine;

        return;
    }

    setProperties(settings, object);

    for (QLoaderSettings *child : hash.data[settings].children)
    {
        if (!status)
            loadRecursive(child, object);
    }
}

bool QLoaderTreePrivate::load()
{
    if (loaded)
        return false;

    loaded = true;

    loadRecursive(root.settings, nullptr);

    if (status && root.object)
        root.object->deleteLater();

    return (status == QLoaderTree::NoError);
}

bool QLoaderTreePrivate::move(const QStringList& /*section*/, const QStringList& /*to*/)
{
    if (loaded)
    {
        modified = true;
        emit q_ptr->settingsChanged();
    }

    return false;
}

void QLoaderTreePrivate::saveRecursive(QLoaderSettings *settings, QTextStream &out)
{
    out << '[' << hash.data[settings].section.join('/') << "]\n" ;
    out << "class = " << hash.data[settings].className << '\n';

    QMapIterator<QString, QVariant> i(hash.data[settings].properties);
    while (i.hasNext())
    {
        i.next();
        out << i.key() << " = " << i.value().toString() << '\n';
    }

    out << '\n';

    for (QLoaderSettings *child : hash.data[settings].children)
    {
        saveRecursive(child, out);
    }
}

bool QLoaderTreePrivate::save()
{
    if (loaded && modified)
    {
        if (!file->open(QIODevice::WriteOnly | QIODevice::Text))
        {
            status = QLoaderTree::AccessError;
            return false;
        }

        QTextStream out(file);

        if (execLine.size())
            out << execLine << '\n';

        saveRecursive(root.settings, out);

        modified = false;
        file->close();

        return true;
    }

    return false;
}
