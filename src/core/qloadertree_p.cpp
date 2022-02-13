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
#include "qloaderplugininterface.h"
#include "qloadermoveinterface.h"
#include "qloadersaveinterface.h"
#include <QRegularExpression>
#include <QFile>
#include <QPluginLoader>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QTextStream>

void QLoaderSettingsData::clear()
{
    parent = nullptr;
    section.clear();
    className.clear();
    object = nullptr;
    properties.clear();
    children.clear();
}

class QLoaderTreeSection
{
    QLoaderTreeSection(const QStringList &section)
    :   section(section)
    {
        if (section.isEmpty())
            return;

        parent.section = section;
        parent.section.removeLast();

        valid = true;
    }

public:
    bool valid{};

    struct
    {
        QStringList section;
        QLoaderSettings *settings{};

    } parent;

    const QStringList &section;
    QLoaderSettings *settings{};
    QObject *object{};

    QLoaderTreeSection(const QStringList &section, QLoaderTreePrivate *d)
    :   QLoaderTreeSection(section)
    {
        if (!valid)
            return;

        if (!d->hash.settings.contains(parent.section))
        {
            valid = false;
            return;
        }

        valid = true;
        parent.settings = d->hash.settings.value(parent.section);
        settings = d->hash.settings.value(section);
        object = d->hash.data.value(settings).object;
    }
};

class KeyValueParser
{
    struct
    {
        const QRegularExpression sectionName{"^\\[(?<section>[^\\[\\]]*)\\]$"};
        const QRegularExpression keyValue{"^(?<key>[^=]*[\\w]+)\\s*=\\s*(?<value>.+)$"};
        const QRegularExpression className{"^[A-Z]+[a-z,0-9]*"};

    } d;

public:
    bool matchSectionName(const QString &line, QStringList &section) const
    {
        QRegularExpressionMatch match;
        if ((match = d.sectionName.match(line)).hasMatch())
        {
            section = match.captured("section").split('/');
            return true;
        }

        return false;
    }

    bool matchKeyValue(const QString &line, QString &key, QString &value) const
    {
        QRegularExpressionMatch match;
        if ((match = d.keyValue.match(line)).hasMatch())
        {
            key = match.captured("key");
            value = match.captured("value");
            return true;
        }

        return false;
    }

    bool matchClassName(const char *name, QString &libraryName) const
    {
        QRegularExpressionMatch match;
        if ((match = d.className.match(name)).hasMatch())
        {
            libraryName += match.captured();
            return true;
        }

        return false;
    }
};

class StringVariantConverter
{
    struct
    {
        const QRegularExpression bytearray{"^QByteArray\\s*\\(\\s*(?<bytearray>.*)\\)"};
        const QRegularExpression chr{"^QChar\\s*\\(\\s*(?<char>.{1})\\s*\\)"};
        const QRegularExpression charlist{"^QCharList\\s*\\(\\s*(?<list>.*)\\)"};
        const QRegularExpression size{"^QSize\\s*\\(\\s*(?<width>\\d+)\\s*\\,\\s*(?<height>\\d+)\\s*\\)"};
        const QRegularExpression stringlist{"^QStringList\\s*\\(\\s*(?<list>.*)\\)"};

    } d;

public:
    QVariant fromString(const QString &value) const
    {
        QRegularExpressionMatch match;
        if ((match = d.bytearray.match(value)).hasMatch())
            return QVariant::fromValue(QByteArray::fromBase64(match.captured("bytearray").toLocal8Bit()));

        if ((match = d.chr.match(value)).hasMatch())
            return match.captured("char").front();

        if ((match = d.charlist.match(value)).hasMatch())
        {
            QStringList stringlist = match.captured("list").split(',');
            QList<QChar> charlist;

            for (QString &string : stringlist)
            {
                string = string.trimmed();
                if (string.size() != 1)
                    return QVariant();

                charlist.append(string.at(0));
            }

            return QVariant::fromValue(charlist);
        }

        if ((match = d.size.match(value)).hasMatch())
            return QSize(match.captured("width").toInt(), match.captured("height").toInt());

        if ((match = d.stringlist.match(value)).hasMatch())
        {
            QStringList stringlist = match.captured("list").split(',');

            for (QString &string : stringlist)
                string = string.trimmed();

            return stringlist;
        }

        return value;
    }

    QString fromVariant(const QVariant &variant) const
    {
        if (variant.canConvert<QByteArray>())
        {
            QByteArray bytearray = variant.toByteArray();
            return QString("QByteArray(" + bytearray.toBase64() + ')');
        }

        if (variant.canConvert<QChar>())
        {
            return QString(QLatin1String("QChar(") + variant.toChar() + ')');
        }

        if (variant.canConvert<QSize>())
        {
            QSize size = variant.toSize();
            return QString("QSize(" + QString::number(size.width()) + ", "
                                    + QString::number(size.height()) + ')');
        }

        return variant.toString();
    }
};

class QLoaderTreePrivateData
{
public:
    QList<QLoaderSettings*> imported;

    struct
    {
        QLoaderSettings *settings{};
        QObject *object{};

    } root;

    KeyValueParser parser;
    StringVariantConverter converter;
};

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   d(*new (&d_storage) QLoaderTreePrivateData),
    q_ptr(q),
    file(new QFile(fileName, q))
{
    static_assert (sizeof (d_storage) == sizeof (QLoaderTreePrivateData));
    static_assert (sizeof (ptrdiff_t) == alignof (QLoaderTreePrivateData));

    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        status = QLoaderTree::AccessError;
        emit q->statusChanged(status);
        return;
    }

    QLoaderSettings *settings{};
    QLoaderSettingsData item;
    errorLine = 0;
    const char comment = '#';

    while (!file->atEnd())
    {
        QByteArray line = file->readLine();
        ++errorLine;

        if (errorLine == 1 && line.startsWith("#!"))
        {
            execLine = line;
            continue;
        }

        if (line == "\n")
            continue;

        if (line.startsWith(comment))
            continue;

        QStringList section;
        if (d.parser.matchSectionName(line, section))
        {
            if (settings)
            {
                hash.data[settings] = item;

                if (item.section.size() == 1 && settings != d.root.settings)
                    d.imported.append(settings);
           }

            settings = new QLoaderSettings(*this);
            bool valid{};
            item.clear();
            item.section = section;

            if (!hash.settings.contains(section))
            {
                if (section.size() == 1 && section.back().size())
                {
                    valid = true;
                }
                else if (d.root.settings && section.size() > 1 && section.back().size())
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
                else
                {
                    errorMessage = "section not valid";
                }
            }
            else
            {
                errorMessage = "section already set";
            }

            if (!valid)
            {
                delete settings;
                qDeleteAll(hash.settings);
                status = QLoaderTree::DesignError;
                emit q->statusChanged(status);
                return;
            }

            hash.settings[section] = settings;
            continue;
        }

        QString key, value;
        if (d.parser.matchKeyValue(line, key, value))
        {
            if (!item.section.size())
            {
                status = QLoaderTree::FormatError;
                errorMessage = "section not set";
                emit q->statusChanged(status);
                return;
            }

            if (key == "class")
            {
                if (item.className.size())
                {
                    status = QLoaderTree::FormatError;
                    errorMessage = "class already set";
                    emit q->statusChanged(status);
                    return;
                }

                if (!d.root.settings && item.section.size() == 1 &&
                                        item.section.back() != value)
                {
                    d.root.settings = settings;
                }

                item.className = value.toLocal8Bit();
                item.classLine = errorLine;
            }
            else
            {
                item.properties[key] = value;
            }
        }
    }

    hash.data[settings] = item;
    errorLine = -1;
    file->close();
}

QLoaderTreePrivate::~QLoaderTreePrivate()
{
    d.~QLoaderTreePrivateData();
}

QObject *QLoaderTreePrivate::builtin(QLoaderSettings* /*settings*/, QObject* /*parent*/)
{
    return nullptr;
}

QObject *QLoaderTreePrivate::external(QLoaderSettings *settings, QObject *parent)
{
    const char *className = settings->className();
    QString libraryName("Qt" + QString::number(QT_VERSION_MAJOR));

    if (d.parser.matchClassName(className, libraryName))
    {
        QPluginLoader loader(libraryName);
        if (!loader.instance())
        {
            status = QLoaderTree::PluginError;
            errorMessage = "library not loaded";
            errorLine = hash.data[settings].classLine;
            emit q_ptr->statusChanged(status);
            return nullptr;
        }

        QLoaderPluginInterface *plugin = qobject_cast<QLoaderPluginInterface*>(loader.instance());
        if (!plugin)
        {
            status = QLoaderTree::PluginError;
            errorMessage = "interface not valid";
            errorLine = hash.data[settings].classLine;
            emit q_ptr->statusChanged(status);
            return nullptr;
        }

        return plugin->object(settings, parent);
    }

    status = QLoaderTree::PluginError;
    errorMessage = "class name not valid";
    errorLine = hash.data[settings].classLine;
    emit q_ptr->statusChanged(status);
    return nullptr;
}

void QLoaderTreePrivate::setProperties(const QLoaderSettingsData &item, QObject *object)
{
    object->setObjectName(item.section.last());

    auto value = [&item, this](const QString &key, const QVariant defaultValue = QVariant())
    {
        if (item.properties.contains(key))
            return fromString(item.properties[key]);

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

    if (!object->isWidgetType())
        return;

    QWidget *widget = qobject_cast<QWidget*>(object);
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

    QMapIterator<QString, QString> i(hash.data[settings].properties);
    while (i.hasNext())
    {
        i.next();
        qDebug().noquote() << i.key() << '=' << i.value();
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
    QLoaderSettingsData &item = hash.data[settings];

    if (item.section.size() == 1 && item.section.last() == item.className)
         return;

    QObject *object;
    if (!qstrncmp(item.className, "Loader", 6))
        object = builtin(settings, parent);
    else
        object = external(settings, parent);

    if (!object)
        return;

    if (object == parent || object == q_ptr ||
        (!object->parent() && ((object->isWidgetType() && item.section.size() > 1) ||
                               !object->isWidgetType())))
    {
        if (object != q_ptr && object == parent)
        {
            errorMessage = "parent object not valid";
        }
        else if (object == q_ptr)
        {
            errorMessage = "class not found";
        }
        else if (!object->parent())
        {
            errorMessage = "parent object not set";
            delete object;
        }

        status = QLoaderTree::ObjectError;
        errorLine = item.classLine;
        emit q_ptr->statusChanged(status);

        return;
    }

    if (!d.root.object && item.section.size() == 1)
        d.root.object = object;

    item.object = object;

    if (status == QLoaderTree::ObjectError)
    {
        errorObject = object;
        errorLine = item.classLine;
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

    setProperties(item, object);

    for (QLoaderSettings *child : item.children)
    {
        if (!status)
            loadRecursive(child, object);
    }
}

bool QLoaderTreePrivate::load()
{
    if (loaded || status)
        return false;

    bool coreApp = !qobject_cast<QApplication*>(QCoreApplication::instance());
    loadRecursive(d.root.settings, coreApp ? q_ptr : nullptr);

    qDeleteAll(hash.settings);

    if (!status)
    {
        loaded = true;
        emit q_ptr->loaded();
    }
    else if (d.root.object)
        delete d.root.object;

    return loaded;
}

bool QLoaderTreePrivate::load(const QStringList & /*section*/)
{
    if (loaded)
        return false;

    return false;
}

QVariant QLoaderTreePrivate::fromString(const QString &value) const
{
    return d.converter.fromString(value);
}

QString QLoaderTreePrivate::fromVariant(const QVariant &variant) const
{
    return d.converter.fromVariant(variant);
}

void QLoaderTreePrivate::copyOrMoveRecursive(QLoaderSettings *settings,
                                             const QLoaderTreeSection &src, const QLoaderTreeSection &dst,
                                             Action action)
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

    if (action == Action::Move)
    {
        hash.settings.remove(item.section);
        item.section = std::move(section);
    }

    hash.settings.insert(item.section, settings);

    for (QLoaderSettings *child : item.children)
        copyOrMoveRecursive(child, src, dst, action);
}

bool QLoaderTreePrivate::copyOrMove(const QStringList &section, const QStringList &to, Action action)
{
    if (!loaded || action == Action::Copy)
        return false;

    QLoaderTreeSection src(section, this);
    if (src.valid)
    {
        QLoaderTreeSection dst(to, this);
        if (dst.valid)
        {
            QLoaderMoveInterface *movable = qobject_cast<QLoaderMoveInterface*>(src.object);
            if (action == Action::Move && (!movable || !movable->move(to)))
            {
                status = QLoaderTree::ObjectError;
                emit q_ptr->statusChanged(status);

                errorObject = src.object;
                if (!movable)
                    errorMessage = "object not movable";
                else
                    errorMessage = "parent object not valid";

                emit q_ptr->errorChanged(errorObject, errorMessage);

                return false;
            }

            std::vector<QLoaderSettings*> &children = hash.data[src.parent.settings].children;
            if (action == Action::Move)
            {
                std::erase(children, src.settings);
                hash.data[dst.parent.settings].children.push_back(src.settings);
            }

            copyOrMoveRecursive(src.settings, src, dst, action);
            modified = true;
            emit q_ptr->settingsChanged();

            return true;
        }
    }

    status = QLoaderTree::DesignError;
    emit q_ptr->statusChanged(status);
    return false;
}

void QLoaderTreePrivate::saveItem(const QLoaderSettingsData &item, QTextStream &out)
{
    out << "\n[" << item.section.join('/') << "]\n" ;
    out << "class = " << item.className << '\n';

    QMapIterator<QString, QString> i(item.properties);
    while (i.hasNext())
    {
        i.next();
        out << i.key() << " = " << i.value() << '\n';
    }

    QLoaderSaveInterface *resources = qobject_cast<QLoaderSaveInterface*>(item.object);
    if (resources)
        resources->save();
}

void QLoaderTreePrivate::saveRecursive(QLoaderSettings *settings, QTextStream &out)
{
    const QLoaderSettingsData &item = hash.data[settings];
    saveItem(item, out);

    for (QLoaderSettings *child : item.children)
        saveRecursive(child, out);
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
            out << execLine;

        for (QLoaderSettings *settings : d.imported)
            saveItem(hash.data[settings], out);

        saveRecursive(d.root.settings, out);

        modified = false;
        file->close();

        return true;
    }

    return false;
}
