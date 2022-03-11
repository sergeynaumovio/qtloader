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

        d->mutex.lock();
        if (!d->hash.settings.contains(parent.section))
            valid = false;
        else
        {
            valid = true;
            parent.settings = d->hash.settings.value(parent.section);
            settings = d->hash.settings.value(section);
            object = d->hash.data.value(settings).object;
        }
        d->mutex.unlock();
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
    struct
    {
        QLoaderSettings *settings{};
        QObject *object{};

    } root;

    KeyValueParser parser;
    StringVariantConverter converter;
    QMutex loading;
    QString execLine;
    QList<QLoaderSettings*> copied;
};

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   d(*new (&d_storage) QLoaderTreePrivateData),
    q_ptr(q),
    file(new QFile(fileName, q))
{
    static_assert (sizeof (d_storage) == sizeof (QLoaderTreePrivateData));
    static_assert (sizeof (ptrdiff_t) == alignof (QLoaderTreePrivateData));
}

QLoaderTreePrivate::~QLoaderTreePrivate()
{
    d.~QLoaderTreePrivateData();
}

QObject *QLoaderTreePrivate::builtin(QLoaderTree::Error & /*error*/,
                                     QLoaderSettings * /*settings*/,
                                     QObject * /*parent*/)
{
    return nullptr;
}

QObject *QLoaderTreePrivate::external(QLoaderTree::Error &error,
                                      QLoaderSettings *settings,
                                      QObject *parent)
{
    QString libraryName("Qt" + QString::number(QT_VERSION_MAJOR));

    if (d.parser.matchClassName(settings->className(), libraryName))
    {
        QPluginLoader loader(libraryName);
        if (!loader.instance())
        {
            mutex.lock();
            error.line = hash.data[settings].sectionLine;
            error.status = QLoaderTree::PluginError;
            error.message = "library not loaded";
            mutex.unlock();
            return nullptr;
        }

        QLoaderPluginInterface *plugin = qobject_cast<QLoaderPluginInterface*>(loader.instance());
        if (!plugin)
        {
            mutex.lock();
            error.line = hash.data[settings].sectionLine;
            error.status = QLoaderTree::PluginError;
            error.message = "interface not valid";
            mutex.unlock();
            return nullptr;
        }

        return plugin->object(settings, parent);
    }

    mutex.lock();
    error.line = hash.data[settings].sectionLine;
    error.status = QLoaderTree::PluginError;
    error.message = "class name not valid";
    mutex.unlock();
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

    QWidget *widget = static_cast<QWidget*>(object);
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
        dumpRecursive(child);
}

void QLoaderTreePrivate::dump(QLoaderSettings *settings) const
{
    if (d.execLine.size())
        qDebug().noquote() << d.execLine;

    dumpRecursive(settings);
}

QLoaderTree::Error QLoaderTreePrivate::loadRecursive(QLoaderSettings *settings, QObject *parent)
{
    QLoaderTree::Error error{};

    mutex.lock();
    QByteArray itemClassName = hash.data[settings].className;
    mutex.unlock();

    QObject *object;
    if (!qstrncmp(itemClassName, "Loader", 6))
        object = builtin(error, settings, parent);
    else
        object = external(error, settings, parent);

    if (!object)
       return error;

    mutex.lock();
    int itemSectionSize = hash.data[settings].section.size();
    int itemSectionLine = hash.data[settings].sectionLine;
    mutex.unlock();

    if (object == parent || object == q_ptr ||
        (!object->parent() && ((object->isWidgetType() && itemSectionSize > 1) ||
                               !object->isWidgetType())))
    {
        error.line = itemSectionLine;
        error.status = QLoaderTree::ObjectError;
        if (object != q_ptr && object == parent)
        {
            error.message = "parent object not valid";
        }
        else if (object == q_ptr)
        {
            error.message = "class not found";
        }
        else if (!object->parent())
        {
            error.message = "parent object not set";
            delete object;
        }

        return error;
    }

    if (!d.root.object && itemSectionSize == 1)
        d.root.object = object;

    mutex.lock();
    hash.data[settings].object = object;
    mutex.unlock();

    if (errorMessage.has_value())
    {
        error.line = itemSectionLine;
        error.status = QLoaderTree::ObjectError;
        error.message = errorMessage.value();
        return error;
    }

    if (infoMessage.has_value())
    {
        QString message = infoMessage.value();
        infoMessage.reset();
        emit q_ptr->infoChanged(object, message);
    }

    if (warningMessage.has_value())
    {
        QString message = warningMessage.value();
        warningMessage.reset();
        emit q_ptr->warningChanged(object, message);
    }

    mutex.lock();
    setProperties(hash.data[settings], object);
    std::vector<QLoaderSettings*> children = hash.data[settings].children;
    mutex.unlock();

    for (QLoaderSettings *child : children)
    {
        error = loadRecursive(child, object);
        if (error.status)
            return error;
    }

    return error;
}

QLoaderTree::Error QLoaderTreePrivate::read()
{
    QLoaderTree::Error error{};
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error.status = QLoaderTree::AccessError;
        error.message = "read error";
        return error;
    }

    struct CloseFile
    {
        QFile *file;
        CloseFile(QFile *f) : file(f) { }
        ~CloseFile(){ file->close(); }

    } closeFile (file);

    QLoaderSettings *settings{};
    QLoaderSettingsData item;
    int errorLine = 0;
    const char comment = '#';

    while (!file->atEnd())
    {
        QByteArray line = file->readLine();
        ++errorLine;

        if (errorLine == 1 && line.startsWith("#!"))
        {
            d.execLine = line;
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
                hash.data[settings] = item;

            settings = new QLoaderSettings(*this);
            bool valid{};
            item.clear();
            item.section = section;

            if (!hash.settings.contains(section))
            {
                if (section.size() == 1 && section.back().size())
                {
                    if (!d.root.settings)
                        valid = true;
                    else
                        error.message = "root object already set";
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
                    error.message = "section not valid";
                }
            }
            else
            {
                error.message = "section already set";
            }

            if (!valid)
            {
                delete settings;
                qDeleteAll(hash.settings);
                error.line = item.sectionLine;
                error.status = QLoaderTree::DesignError;
                return error;
            }

            hash.settings[section] = settings;
            item.sectionLine = errorLine;
            continue;
        }

        QString key, value;
        if (d.parser.matchKeyValue(line, key, value))
        {
            if (!item.section.size())
            {
                error.line = item.sectionLine;
                error.status = QLoaderTree::FormatError;
                error.message = "section not set";
                return error;
            }

            if (key == "class")
            {
                if (item.className.size())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderTree::FormatError;
                    error.message = "class already set";
                    return error;
                }

                if (!d.root.settings && item.section.size() == 1)
                {
                    d.root.settings = settings;
                }

                item.className = value.toLocal8Bit();
            }
            else
            {
                item.properties[key] = value;
            }
        }
    }

    hash.data[settings] = item;

    return {};
}

QLoaderTree::Error QLoaderTreePrivate::load()
{
    QLoaderTree::Error error;
    d.loading.lock();
    {
        if (loaded)
        {
            error.status = QLoaderTree::ObjectError;
            error.message = "already loaded";
            d.loading.unlock();
            return error;
        }

        error = read();
        if (error.status)
        {
            d.loading.unlock();
            return error;
        }

        bool coreApp = !qobject_cast<QApplication*>(QCoreApplication::instance());
        error = loadRecursive(d.root.settings, coreApp ? q_ptr : nullptr);

        qDeleteAll(hash.settings);

        if (!error.status)
            loaded = true;
        else if (d.root.object)
            delete d.root.object;
    }
    d.loading.unlock();

    if (loaded)
        emit q_ptr->loaded();

    return error;
}

QLoaderTree::Error QLoaderTreePrivate::load(const QStringList & /*section*/)
{
    if (loaded)
        return {};

    return {};
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
                                             const QLoaderTreeSection &src,
                                             const QLoaderTreeSection &dst,
                                             Action action)
{
    QLoaderSettingsData &item = hash.data[settings];

    int size = item.section.size() + (dst.section.size() - src.section.size());
    QStringList section(size);
    std::copy_n(dst.section.begin(), dst.section.size(), section.begin());
    std::copy_n(item.section.begin() + src.section.size(),
                item.section.size() - src.section.size(),
                section.begin() + dst.section.size());

    if (action == Action::Move)
    {
        hash.settings.remove(item.section);
        hash.settings[section] = settings;
        item.section = std::move(section);
    }
    else
    {
        QLoaderSettings *copySettings = new QLoaderSettings(*this);
        d.copied.append(copySettings);
        hash.settings[section] = copySettings;

        QStringList parentSection = section;
        parentSection.removeLast();

        QLoaderSettings *parentSettings = hash.settings[parentSection];

        hash.data[parentSettings].children.push_back(copySettings);

        QLoaderSettingsData &copy = hash.data[copySettings];
        copy.parent = parentSettings;
        copy.section = std::move(section);
        copy.className = hash.data[settings].className;
        copy.properties = hash.data[settings].properties;
    }

    for (QLoaderSettings *child : hash.data[settings].children)
        copyOrMoveRecursive(child, src, dst, action);
}

QLoaderTree::Error QLoaderTreePrivate::copyOrMove(const QStringList &section, const QStringList &to, Action action)
{
    QLoaderTree::Error error;
    if (!loaded)
    {
        error.status = QLoaderTree::ObjectError;
        error.message = "tree not loaded";
        return error;
    }

    QLoaderTreeSection src(section, this);
    if (src.valid)
    {
        QLoaderTreeSection dst(to, this);
        if (dst.valid)
        {
            if (action == Action::Move)
            {
                QLoaderMoveInterface *movable = qobject_cast<QLoaderMoveInterface*>(src.object);
                if (!movable || !movable->move(to))
                {
                    if (!movable)
                        error.message = "object not movable";
                    else
                        error.message = "parent object not valid";

                    error.status = QLoaderTree::ObjectError;
                    emit q_ptr->errorChanged(src.object, error.message);

                    return error;
                }

                mutex.lock();
                std::vector<QLoaderSettings*> &children = hash.data[src.parent.settings].children;
                std::erase(children, src.settings);
                hash.data[dst.parent.settings].children.push_back(src.settings);
                mutex.unlock();
            }

            mutex.lock();
            copyOrMoveRecursive(src.settings, src, dst, action);
            mutex.unlock();

            if (action == Action::Copy)
            {
                error = loadRecursive(hash.settings[dst.section],
                                      hash.data[hash.settings[dst.parent.section]].object);

                mutex.lock();
                qDeleteAll(d.copied);
                d.copied.clear();
                mutex.unlock();

                if (error.status)
                {
                    mutex.lock();
                    removeRecursive(dst.settings);
                    mutex.unlock();

                    return error;
                }
            }

            modified = true;
            emit q_ptr->settingsChanged();

            return error;
        }
    }

    error.status = QLoaderTree::DesignError;
    error.message = "section not valid";
    return error;
}

void QLoaderTreePrivate::removeRecursive(QLoaderSettings */*settings*/)
{

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

QLoaderTree::Error QLoaderTreePrivate::save()
{
    QLoaderTree::Error error;
    if (loaded)
    {
        if (!file->open(QIODevice::WriteOnly | QIODevice::Text))
        {
            error.status = QLoaderTree::AccessError;
            error.message = "read-only file";
            return error;
        }

        QTextStream out(file);

        if (d.execLine.size())
            out << d.execLine;

        saveRecursive(d.root.settings, out);

        file->close();
        modified = false;

        return {};
    }

    error.status = QLoaderTree::ObjectError;
    error.message = "tree not loaded";
    return error;
}
