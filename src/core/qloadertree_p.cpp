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

#include "qloadertree_p.h"
#include "qloadertree.h"
#include "qloadersettings.h"
#include "qloaderinterface.h"
#include <QRegularExpression>
#include <QFile>
#include <QPluginLoader>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QTextStream>

class StringVariantConverter
{
    const QRegularExpression size;

    QRegularExpressionMatch match;

public:
    StringVariantConverter()
    :   size("^QSize\\s*\\(\\s*(?<width>\\d+)\\s*\\,\\s*(?<height>\\d+)\\s*\\)")
    { }

    QVariant fromString(const QString &value)
    {
        if ((match = size.match(value)).hasMatch())
            return QSize(match.captured("width").toInt(), match.captured("height").toInt());

        return value;
    }

    QString fromVariant(const QVariant &variant)
    {
        if (variant.canConvert<QSize>())
        {
            QSize size = variant.toSize();
            return QString("QSize(" + QString::number(size.width()) + ", "
                                    + QString::number(size.height()) + ')');
        }

        return variant.toString();
    }
};

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   converter(new StringVariantConverter),
    q_ptr(q),
    file(new QFile(fileName, q))

{
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        status = QLoaderTree::AccessError;
        emit q->statusChanged(status);
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
            bool valid{};
            QLoaderSettingsData item;
            item.section = section;

            if (!root.settings && section.size() == 1 && section.back().size())
            {
                root.settings = settings;
                valid = true;
            }
            else if (root.settings && section.size() > 1 && section.back().size() &&
                     !hash.settings.contains(section))
            {
                QStringList parent = section;
                parent.removeLast();

                if (hash.settings.contains(parent))
                {
                    valid = true;
                    item.parent = hash.settings[parent];
                    hash.data[item.parent].children.push_back(settings);
                }
            }

            if (!valid)
            {
                status = QLoaderTree::DesignError;
                emit q->statusChanged(status);
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
            QVariant value = fromString(keyValueMatch.captured("value"));

            if (key == "class")
            {
                if (!settings)
                {
                    status = QLoaderTree::FormatError;
                    emit q->statusChanged(status);
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
            errorMessage = "library not loaded";
            emit q_ptr->statusChanged(status);
            return nullptr;
        }

        QLoaderInterface *plugin = qobject_cast<QLoaderInterface*>(loader.instance());
        if (!plugin)
        {
            status = QLoaderTree::PluginError;
            errorMessage = "interface not valid";
            emit q_ptr->statusChanged(status);
            return nullptr;
        }

        return plugin->object(settings, parent);
    }

    return nullptr;
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

        if (!(v = value("fixedHeight")).isNull())
            widget->setFixedHeight(v.toInt());

        if (!(v = value("fixedSize")).isNull())
            widget->setFixedSize(v.toSize());

        if (!(v = value("fixedWidth")).isNull())
            widget->setFixedWidth(v.toInt());

        if (!(v = value("hidden")).isNull())
            widget->setHidden(v.toBool());

        if (!(v = value("maximumHeight")).isNull())
            widget->setMaximumHeight(v.toInt());

        if (!(v = value("maximumSize")).isNull())
            widget->setMaximumSize(v.toSize());

        if (!(v = value("maximumWidth")).isNull())
            widget->setMaximumWidth(v.toInt());

        if (!(v = value("minimumHeight")).isNull())
            widget->setMinimumHeight(v.toInt());

        if (!(v = value("minimumSize")).isNull())
            widget->setMinimumSize(v.toSize());

        if (!(v = value("minimumWidth")).isNull())
            widget->setMinimumWidth(v.toInt());

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

    if (!object || object == parent || status == QLoaderTree::PluginError)
    {
        if (!status)
        {
            if (!object)
            {
                errorMessage = "class not found";
                status = QLoaderTree::ObjectError;
                emit q_ptr->statusChanged(status);
            }
            else if (object && object == parent)
            {
                errorMessage = "parent object not valid";
                status = QLoaderTree::ObjectError;
                emit q_ptr->statusChanged(status);
            }
        }

        errorLine = hash.data[settings].classLine;

        return;
    }

    if (status == QLoaderTree::ObjectError)
    {
        emit q_ptr->statusChanged(status);
        emit q_ptr->errorChanged(object, errorMessage);
        return;
    }

    if (infoChanged)
    {
        infoObject = object;
        infoChanged = false;
        emit q_ptr->infoChanged(object, infoMessage);
    }

    if (warningChanged)
    {
        warningObject = object;
        warningChanged = false;
        emit q_ptr->warningChanged(object, warningMessage);
    }

    hash.data[settings].object = object;
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

bool QLoaderTreePrivate::load(const QStringList & /*section*/)
{
    if (loaded)
        return false;

    return false;
}

Section::Section(const QStringList &section)
:   section(section)
{
    if (section.isEmpty())
        return;

    parent.section = section;
    parent.section.removeLast();

    valid = true;
}

Section::Section(const QStringList &section, QLoaderTreePrivate *d)
:   Section(section)
{
    if (!valid)
        return;

    if (!d->hash.settings.contains(parent.section))
    {
        valid = false;
        return;
    }

    settings = d->hash.settings.value(section);
    parent.settings = d->hash.settings.value(parent.section);
    valid = true;
}

QVariant QLoaderTreePrivate::fromString(const QString &value)
{
    return converter->fromString(value);
}

QString QLoaderTreePrivate::fromVariant(const QVariant &variant)
{
    return converter->fromVariant(variant);
}

void QLoaderTreePrivate::copyOrMoveRecursive(QLoaderSettings *settings,
                                             const Section &src, const Section &dst,
                                             Section::Instance instance)
{
    QLoaderSettingsData &item = hash.data[settings];

    QStringList section = [&src, &dst](const QLoaderSettingsData &item)
    {
        int d = dst.section.size() - src.section.size();
        int size = item.section.size() + d;
        QStringList section(size);

        std::copy_n(dst.section.begin(), dst.section.size(), section.begin());
        std::copy_n(item.section.begin() + src.section.size(),
                    item.section.size() - src.section.size(),
                    section.begin() + dst.section.size());

        return section;

    }(hash.data[settings]);

    if (instance == Section::Move)
    {
        hash.settings.remove(item.section);
        item.section = std::move(section);
    }

    hash.settings.insert(item.section, settings);

    for (QLoaderSettings *child : item.children)
        copyOrMoveRecursive(child, src, dst, instance);
}

bool QLoaderTreePrivate::copyOrMove(const QStringList &section, const QStringList &to, Section::Instance instance)
{
    if (!loaded || instance == Section::Copy)
        return false;

    Section src(section, this);
    if (src.valid)
    {
        Section dst(to, this);
        if (dst.valid)
        {
            std::vector<QLoaderSettings*> &children = hash.data[src.parent.settings].children;
            for (int i = 0; i < static_cast<int>(children.size()); ++i)
            {
                if (children[i] == src.settings)
                {
                    if (instance == Section::Move)
                    {
                        children.erase(children.begin() + i);
                        hash.data[dst.parent.settings].children.push_back(src.settings);
                    }

                    copyOrMoveRecursive(src.settings, src, dst, instance);
                    modified = true;
                    emit q_ptr->settingsChanged();
                    return true;
                }
            }
        }
    }

    status = QLoaderTree::DesignError;
    emit q_ptr->statusChanged(status);
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
        out << i.key() << " = " << fromVariant(i.value()) << '\n';
    }

    for (QLoaderSettings *child : hash.data[settings].children)
    {
        out << '\n';
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
            emit q_ptr->statusChanged(status);
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
