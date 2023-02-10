// Copyright (C) 2023 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadertree_p.h"
#include "qloaderdata.h"
#include "qloaderdir.h"
#include "qloadertree.h"
#include "qloaderplugininterface.h"
#include "qloadersaveinterface.h"
#include "qloadersettings.h"
#include "qloadershell.h"
#include "qloadershellcd.h"
#include "qloadershellclear.h"
#include "qloadershellexit.h"
#include "qloadershellsave.h"
#include "qloaderstorage.h"
#include "qloaderstorage_p.h"
#include "qloaderterminal.h"
#include <QAction>
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMetaMethod>
#include <QPluginLoader>
#include <QRegularExpression>
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

enum Action { Copy, Move };

template<Action>
class QLoaderTreeSectionAction
{
    QLoaderTreePrivate *const d_ptr;

    bool allow() const;

public:
    QLoaderTreeSection src;
    QLoaderTreeSection dst;

    QLoaderTreeSectionAction(const QStringList &srcPath,
                             const QStringList &dstPath,
                             QLoaderTreePrivate *d)
    :   d_ptr(d),
        src(srcPath, d),
        dst(dstPath, d)
    { }

    QLoaderError error()
    {
        d_ptr->mutex.lock();
        bool loaded = d_ptr->loaded;
        d_ptr->mutex.unlock();
        if (!loaded)
            return {.status = QLoaderError::Object, .message = "tree not loaded"};

        if (!src.valid || !dst.valid)
            return {.status = QLoaderError::Design, .message = "section not valid"};

        if (!allow())
        {
            QLoaderError err;
            err.message = "operation not allowed";
            err.status = QLoaderError::Object;
            emit d_ptr->q_ptr->errorChanged(src.object, err.message);

            return err;
        }

        return {};
    }

    static QStringList section(const QStringList &itemSection,
                               const QStringList &srcSection,
                               const QStringList &dstSection)
    {
        int size = itemSection.size() + (dstSection.size() - srcSection.size());
        QStringList retSection(size);
        std::copy_n(dstSection.begin(), dstSection.size(), retSection.begin());
        std::copy_n(itemSection.begin() + srcSection.size(),
                    itemSection.size() - srcSection.size(),
                    retSection.begin() + dstSection.size());

        return retSection;
    }
};

template<>
bool QLoaderTreeSectionAction<Copy>::allow() const
{
    return d_ptr->hash.data[src.settings].settings.front()->isCopyable(dst.section);
}

template<>
bool QLoaderTreeSectionAction<Move>::allow() const
{
    return d_ptr->hash.data[src.settings].settings.front()->isMovable(dst.section);
}

class KeyValueParser
{
    struct
    {
        const QRegularExpression sectionName{"^\\[(?<section>[^\\[\\]]*)\\]$"};
        const QRegularExpression keyValue{"^(?<key>[^=]*[^\\s])\\s*=\\s*(?<value>.+)"};
        const QRegularExpression className{"^[A-Z]+[a-z,0-9]*"};
        const QRegularExpression blobUuid{"^QLoaderBlob\\((?<uuid>[0-9a-f\\-]{36})\\)"};

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

    bool matchBlobUuid(const QString &value, QString &uuid) const
    {
        QRegularExpressionMatch match;
        if ((match = d.blobUuid.match(value)).hasMatch())
        {
            uuid = match.captured("uuid");
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
        const QRegularExpression blob{"^QLoaderBlob\\s*\\(\\s*(?<bytearray>.*)\\)"};
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
        if ((match = d.blob.match(value)).hasMatch())
            return QVariant::fromValue(match.captured("bytearray").toLocal8Bit());

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
        QMetaType::Type type = static_cast<QMetaType::Type>(variant.metaType().id());
        if (type == QMetaType::QByteArray)
        {
            QByteArray bytearray = variant.toByteArray();
            return QString("QByteArray(" + bytearray.toBase64() + ')');
        }

        if (type == QMetaType::QChar)
        {
            return QString(QLatin1String("QChar(") + variant.toChar() + ')');
        }

        if (type == QMetaType::QSize)
        {
            QSize size = variant.toSize();
            return QString("QSize(" + QString::number(size.width()) + ", "
                                    + QString::number(size.height()) + ')');
        }

        return variant.toString();
    }
};

class Saving
{
    Saving *s{};
    QMutex mutex;
    bool inProgress{};

public:
    Saving() { }

    Saving(Saving *saving)
    :   s(saving)
    {
        s->mutex.lock();
        s->inProgress = true;
        s->mutex.unlock();
    }

    ~Saving()
    {
        if (s)
        {
            s->mutex.lock();
            s->inProgress = false;
            s->mutex.unlock();
        }
    }

    bool operator ()()
    {
        mutex.lock();
        bool ret = inProgress;
        mutex.unlock();

        return ret;
    }
};

class SettingsObject
{
public:
    QLoaderSettings *settings{};
    QObject *object{};
};

class StorageSettingsObject : public SettingsObject
{
public:
    QLoaderStoragePrivate *d_ptr{};
};

class QLoaderTreePrivateData
{
public:
    SettingsObject root;
    SettingsObject data;
    SettingsObject shell;
    StorageSettingsObject storage;
    QSet<QUuid> blobs;

    KeyValueParser parser;
    StringVariantConverter converter;
    QMutex loading;
    QString execLine;
    QList<QLoaderSettings*> copied;
    Saving saving;

    ~QLoaderTreePrivateData()
    {
        if (shell.object) delete shell.object;
        else if (shell.settings) delete shell.settings;
    }
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

QLoaderBlob QLoaderTreePrivate::blob(const QUuid &uuid) const
{
    return d.storage.d_ptr->blob(uuid);
}

QObject *QLoaderTreePrivate::builtin(QLoaderSettings *settings, QObject *parent)
{
    QByteArray className = settings->className();
    const char *shortName = className.data() + qstrlen("QLoader");

    if (!qstrcmp(shortName, "ShellCd"))
    {
        QLoaderShell *shell = qobject_cast<QLoaderShell*>(parent);
        if (shell)
            return new QLoaderShellCd(settings, shell);

        return parent;
     }

    if (!qstrcmp(shortName, "ShellClear"))
    {
        QLoaderShell *shell = qobject_cast<QLoaderShell*>(parent);
        if (shell)
            return new QLoaderShellClear(settings, shell);

        return parent;
     }

    if (!qstrcmp(shortName, "Data"))
    {
        if (!d.data.object)
            return d.data.object = new QLoaderData(settings, parent);

        d.data.object->setParent(parent);
        return nullptr;
    }

    if (!qstrcmp(shortName, "Dir"))
    {
        QLoaderData *ldata = qobject_cast<QLoaderData*>(parent);
        if (ldata)
            return new QLoaderDir(settings, ldata);

        QLoaderDir *dir = qobject_cast<QLoaderDir*>(parent);
        if (dir)
            return new QLoaderDir(settings, dir);

        return parent;
    }

    if (!qstrcmp(shortName, "ShellExit"))
    {
        QLoaderShell *shell = qobject_cast<QLoaderShell*>(parent);
        if (shell)
            return new QLoaderShellExit(settings, shell);

        return parent;
     }

    if (!qstrcmp(shortName, "ShellSave"))
    {
        QLoaderShell *shell = qobject_cast<QLoaderShell*>(parent);
        if (shell)
            return new QLoaderShellSave(settings, shell);

        return parent;
     }

    if (!qstrcmp(shortName, "Shell"))
    {
        if (!d.shell.object)
            return d.shell.object = new QLoaderShell(settings);

        return nullptr;
     }

    if (!qstrcmp(shortName, "Storage"))
    {
        if (!d.storage.object)
            return d.storage.object = new QLoaderStorage(*this, settings, parent);

        d.storage.object->setParent(parent);
        return nullptr;
     }

    if (!qstrcmp(shortName, "Terminal"))
    {
        bool coreApp = !qobject_cast<QApplication*>(QCoreApplication::instance());
        if (coreApp)
            return nullptr;

        QWidget *widget = qobject_cast<QWidget*>(parent);
        if (!parent || (parent && widget))
            return new QLoaderTerminal(settings, widget);

        return parent;
    }

    return q_ptr;
}

QObject *QLoaderTreePrivate::external(QLoaderError &error,
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
            error.status = QLoaderError::Plugin;
            error.message = "library \"" + libraryName + "\" not loaded";
            mutex.unlock();
            return nullptr;
        }

        QLoaderPluginInterface *plugin = qobject_cast<QLoaderPluginInterface*>(loader.instance());
        if (!plugin)
        {
            mutex.lock();
            error.line = hash.data[settings].sectionLine;
            error.status = QLoaderError::Plugin;
            error.message = "interface not valid";
            mutex.unlock();
            return nullptr;
        }

        return plugin->object(settings, parent);
    }

    mutex.lock();
    error.line = hash.data[settings].sectionLine;
    error.status = QLoaderError::Plugin;
    error.message = "class name \"" + hash.data[settings].className + "\" not valid";

    mutex.unlock();
    return nullptr;
}

QUuid QLoaderTreePrivate::createStorageUuid() const
{
    if (d.storage.d_ptr)
        return d.storage.d_ptr->createUuid();

    return {};
}

QLoaderData *QLoaderTreePrivate::data() const
{
    return static_cast<QLoaderData*>(d.data.object);
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

void QLoaderTreePrivate::emitSettingsChanged()
{
    mutex.lock();
    modified = true;
    mutex.unlock();

    emit q_ptr->settingsChanged();
}

void QLoaderTreePrivate::dumpRecursive(QLoaderSettings *settings) const
{
    qDebug().noquote().nospace() << '[' << hash.data[settings].section.join('/') << ']';
    qDebug().noquote() << "class =" << hash.data[settings].className;

    QMapIterator<QString, QLoaderProperty> i(hash.data[settings].properties);
    while (i.hasNext())
    {
        i.next();
        qDebug().noquote() << i.key() << '=' << i.value().string;
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

QLoaderError QLoaderTreePrivate::loadRecursive(QLoaderSettings *settings, QObject *parent)
{
    QLoaderError error;

    mutex.lock();
    QByteArray itemClassName = hash.data[settings].className;
    mutex.unlock();

    QObject *object;
    if (!qstrncmp(itemClassName, "QLoader", 7))
        object = builtin(settings, parent);
    else
        object = external(error, settings, parent);

    if (!object)
       return error;

    mutex.lock();
    int itemSectionSize = hash.data[settings].section.size();
    int itemSectionLine = hash.data[settings].sectionLine;
    mutex.unlock();

    if (object == parent || object == q_ptr ||
        (object != d.shell.object && !object->parent() &&
         ((object->isWidgetType() && itemSectionSize > 1) || !object->isWidgetType())))
    {
        error.line = itemSectionLine;
        error.status = QLoaderError::Object;
        if (object != q_ptr && object == parent)
        {
            error.message = "parent object not valid";
        }
        else if (object == q_ptr)
        {
            error.message = "class \"" + itemClassName + "\" not found";
        }
        else if (object != d.shell.object && !object->parent())
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

    mutex.lock();
    if (errorMessage.has_value())
    {
        error.line = itemSectionLine;
        error.status = QLoaderError::Object;
        error.message = errorMessage.value();
        mutex.unlock();
        return error;
    }
    else
        mutex.unlock();

    mutex.lock();
    if (infoMessage.has_value())
    {
        QString message = infoMessage.value();
        infoMessage.reset();
        mutex.unlock();
        emit q_ptr->infoChanged(object, message);
    }
    else
        mutex.unlock();

    mutex.lock();
    if (warningMessage.has_value())
    {
        QString message = warningMessage.value();
        warningMessage.reset();
        mutex.unlock();
        emit q_ptr->warningChanged(object, message);
    }
    else
        mutex.unlock();

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

void QLoaderTreePrivate::moveRecursive(QLoaderSettings *settings,
                                       const QLoaderTreeSection &src,
                                       const QLoaderTreeSection &dst)
{
    QLoaderSettingsData &item = hash.data[settings];

    QStringList section = QLoaderTreeSectionAction<Move>::section(item.section, src.section, dst.section);

    hash.settings.remove(item.section);
    hash.settings[section] = settings;
    item.section = std::move(section);

    for (QLoaderSettings *child : hash.data[settings].children)
        moveRecursive(child, src, dst);
}

QLoaderShell *QLoaderTreePrivate::newShellInstance()
{
    if (!d.shell.object)
    {
        if (!d.shell.settings)
        {
            d.shell.settings = new QLoaderSettings(*this);
            hash.data[d.shell.settings] = {};
        }
        QLoaderShell *const shell = new QLoaderShell(d.shell.settings);
        emit q_ptr->errorChanged(shell, "shell not loaded");
        return shell;
    }

    QLoaderShell *const shell = new QLoaderShell(hash.data[d.shell.settings].settings.front());
    mutex.lock();
    const int settings_size = int(hash.data[d.shell.settings].children.size());
    mutex.unlock();

    for (int o = 0, s = 0; s < settings_size; ++o, ++s)
    {
        mutex.lock();
        QObject *object = d.shell.object->children()[o];
        QLoaderSettings *settings = hash.data[hash.data[d.shell.settings].children[s]].settings.front();
        mutex.unlock();

        if (qobject_cast<QLoaderCommandInterface *>(object))
        {
            if (!object->metaObject()->newInstance(Q_ARG(QLoaderSettings*, settings), Q_ARG(QLoaderShell*, shell)))
                emit q_ptr->errorChanged(object, "constructor not invokable");
        }
        else
            emit q_ptr->errorChanged(object, "interface not found");
    }

    return shell;
}

QLoaderError QLoaderTreePrivate::seekBlobs()
{
    QLoaderError error{.line = hash.data[d.storage.settings].sectionLine,
                       .status = QLoaderError::Format};
    QDataStream in(file);
    QUuid currentUuid;
    const int uuidStringSize = currentUuid.toString(QUuid::WithoutBraces).size();
    const QString format(" = QLoaderBlob(");

    auto errMessage = [&error](const QUuid uuid)
    {
        error.message =  "blob uuid " + uuid.toString() + " not valid";
        return error;
    };

    while (!file->atEnd())
    {
        QByteArray array = file->read(uuidStringSize);
        QUuid uuid(array);

        if (!uuid.isNull())
            currentUuid = uuid;

        array = file->read(format.size());

        if (array != format)
            return errMessage(currentUuid);

        qint64 pos = file->pos();
        hash.blobs.insert(uuid, pos);
        qint64 size;
        in >> size;

        pos = file->pos() + size;
        if (pos > file->size())
            return errMessage(currentUuid);

        if (!file->seek(file->pos() + size))
            return errMessage(currentUuid);
    }

    file->close();
    if (!file->open(QIODevice::ReadOnly))
    {
        error.status = QLoaderError::Access;
        error.message = "read error";
        return error;
    }

    return {};
}

QLoaderError QLoaderTreePrivate::readSettings()
{
    QLoaderError error;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error.status = QLoaderError::Access;
        error.message = "read error";
        return error;
    }

    QLoaderSettings *settings{};
    QLoaderSettingsData item;
    int currentLine = 0;
    const char comment = '#';

    while (!file->atEnd())
    {
        QString line = file->readLine();
        ++currentLine;

        if (currentLine == 1 && line.startsWith("#!"))
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
            {
                if (item.className.isEmpty())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Design;
                    error.message = "class name not set";
                    break;
                }
                hash.data[settings] = item;
            }

            settings = new QLoaderSettings(*this);

            bool valid{};
            item.clear();
            item.section = section;
            item.sectionLine = currentLine;

            if (!hash.settings.contains(section))
            {
                hash.settings[section] = settings;
                vector.settings.push_back(settings);

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
                        if (parent.size() == 1 && d.storage.settings)
                        {
                            error.message = "last direct root child not valid (please, use QLoaderStorage)";
                        }
                        else
                        {
                            valid = true;
                            item.parent = hash.settings[parent];
                            hash.data[item.parent].children.push_back(settings);
                        }
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
                delete settings;
            }

            if (!valid)
            {
                error.line = currentLine;
                error.status = QLoaderError::Design;
                if (error.message.isEmpty())
                    error.message = "section not valid";

                break;
            }

            continue;
        }

        QString key, value;
        if (d.parser.matchKeyValue(line, key, value))
        {
            if (item.section.isEmpty())
            {
                error.line = item.sectionLine;
                error.status = QLoaderError::Format;
                error.message = "section not set";
                break;
            }

            if (key == "class")
            {
                if (item.className.size())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Format;
                    error.message = "class already set";
                    break;
                }

                if (!d.root.settings && item.section.size() == 1)
                {
                    d.root.settings = settings;
                }

                item.className = value.toLocal8Bit();
                if (!qstrncmp(item.className, "Loader", 6))
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Object;
                    error.message = "class not found";
                    break;
                }

                bool isData{};
                bool isShell{};
                bool isStorage{};
                const char *shortName = item.className.data() + qstrlen("QLoader");
                if ((isData = !qstrcmp(shortName, "Data")) ||
                    (isShell = !qstrcmp(shortName, "Shell")) ||
                    (isStorage = !qstrcmp(shortName, "Storage")))
                {
                    if (item.section.size() > 2)
                    {
                        error.line = item.sectionLine;
                        error.status = QLoaderError::Design;
                        error.message = "parent object not valid";
                        break;
                    }

                    if ((isData && d.data.settings) ||
                        (isShell && d.shell.settings) ||
                        (isStorage && d.storage.settings))
                    {
                        error.line = item.sectionLine;
                        error.status = QLoaderError::Design;
                        error.message = (isData ? "data" :
                                         isShell ? "shell" :
                                         isStorage ? "storage" : "");
                        error.message += " object already set";
                        break;
                    }

                    if (isData)
                        d.data.settings = settings;
                    else if (isShell)
                        d.shell.settings = settings;
                    else if (isStorage)
                    {
                        d.storage.settings = settings;
                        break;
                    }
                }
            }
            else if (!item.properties.contains(key))
            {
                item.properties[key] = value;

                if (d.parser.matchBlobUuid(value, value))
                {
                    QUuid uuid(value);
                    if (uuid.isNull())
                    {
                        error.line = item.sectionLine;
                        error.status = QLoaderError::Format;
                        error.message = "blob uuid \"" + value + "\" not valid";
                        break;
                    }

                    if (d.blobs.contains(uuid))
                    {
                        error.status = QLoaderError::Design;
                        error.message = "blob uuid\"" + uuid.toString() + "\" already set";
                    }

                    d.blobs.insert(uuid);
                    item.properties[key].isBlob = true;
                }
                else
                    item.properties[key].isValue = true;
            }
            else
            {
                error.status = QLoaderError::Design;
                error.message = "key \"" + key + "\" already set";
                break;
            }
        }
    }

    hash.data[settings] = item;

    if (d.storage.settings)
        return seekBlobs();

    return error;
}

QLoaderError QLoaderTreePrivate::load()
{
    QLoaderError error;
    d.loading.lock();
    do
    {
        if (loaded)
        {
            error.status = QLoaderError::Object;
            error.message = "already loaded";
            break;
        }

        if ((error = readSettings()))
            break;

        if (d.storage.settings && (error = loadRecursive(d.storage.settings, q_ptr)))
            break;

        if (d.shell.settings && (error = loadRecursive(d.shell.settings, q_ptr)))
            break;

        if (d.data.settings && (error = loadRecursive(d.data.settings, q_ptr)))
            break;

        bool coreApp = !qobject_cast<QApplication*>(QCoreApplication::instance());
        if ((error = loadRecursive(d.root.settings, coreApp ? q_ptr : nullptr)))
            break;

        loaded = true;

    } while (0);

    if (!loaded)
    {
        file->close();
        if (d.storage.object) delete d.storage.object;
        if (d.data.object) delete d.data.object;
        if (d.root.object) delete d.root.object;
    }
    else if (!d.storage.object)
        file->close();

    d.loading.unlock();

    if (loaded)
        emit q_ptr->loaded();

    return error;
}

QLoaderError QLoaderTreePrivate::move(const QStringList &section, const QStringList &to)
{
    QLoaderTreeSectionAction<Move> mv(section, to, this);

    QLoaderError error;
    if ((error = mv.error()))
        return error;

    mutex.lock();
    std::erase(hash.data[mv.src.parent.settings].children, mv.src.settings);
    hash.data[mv.dst.parent.settings].children.push_back(mv.src.settings);
    moveRecursive(mv.src.settings, mv.src, mv.dst);
    mutex.unlock();

    emitSettingsChanged();

    return {};
}

QLoaderError QLoaderTreePrivate::load(const QStringList &/*section*/)
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

void QLoaderTreePrivate::copyRecursive(QLoaderSettings *settings,
                                       const QLoaderTreeSection &src,
                                       const QLoaderTreeSection &dst)
{
    QLoaderSettingsData &item = hash.data[settings];

    QStringList section = QLoaderTreeSectionAction<Copy>::section(item.section, src.section, dst.section);

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

    for (QLoaderSettings *child : hash.data[settings].children)
        copyRecursive(child, src, dst);
}

QLoaderError QLoaderTreePrivate::copy(const QStringList &section, const QStringList &to)
{
    QLoaderTreeSectionAction<Copy> cp(section, to, this);

    QLoaderError error;
    if ((error = cp.error()))
        return error;

    mutex.lock();
    copyRecursive(cp.src.settings, cp.src, cp.dst);
    QLoaderSettings *settings = hash.settings[cp.dst.section];
    QObject *parent = hash.data[hash.settings[cp.dst.parent.section]].object;
    mutex.unlock();

    error = loadRecursive(settings, parent);

    mutex.lock();
    qDeleteAll(d.copied);
    d.copied.clear();
    mutex.unlock();

    if (error.status)
    {
        mutex.lock();
        removeRecursive(cp.dst.settings);
        mutex.unlock();

        return error;
    }

    emitSettingsChanged();

    return {};
}

bool QLoaderTreePrivate::isSaving() const
{
    return d.saving();
}

void QLoaderTreePrivate::removeRecursive(QLoaderSettings */*settings*/)
{

}

void QLoaderTreePrivate::saveItem(const QLoaderSettingsData &item, QTextStream &out)
{
    out << "\n[" << item.section.join('/') << "]\n" ;
    out << "class = " << item.className << '\n';

    QMapIterator<QString, QLoaderProperty> i(item.properties);
    while (i.hasNext())
    {
        i.next();
        out << i.key() << " = " << i.value().string << '\n';
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

QLoaderError QLoaderTreePrivate::save()
{
    QLoaderError error{.status = QLoaderError::Access, .message = "read-only file"};
    if (loaded)
    {
        Saving saving(&d.saving);

        QFileDevice::Permissions permissions = file->permissions();

        QString fileName = file->fileName();
        QString bakFileName = fileName + ".bak";

        QFile ofile(bakFileName);
        if (!ofile.open(QIODevice::WriteOnly))
            return error;

        QTextStream out(&ofile);
        if (d.execLine.size())
            out << d.execLine;

        saveRecursive(d.root.settings, out);
        ofile.close();

        if (d.storage.object)
            if (QLoaderError e = d.storage.d_ptr->save(&ofile, d.root.settings))
                return e;

        file->close();
        if (!file->remove())
            return error;

        ofile.rename(fileName);

        if (!file->open(QIODevice::ReadOnly))
            return error;

        file->setPermissions(permissions);

        modified = false;
    }
    else
    {
        error.status = QLoaderError::Object;
        error.message = "tree not loaded";
    }

    return error;
}

void QLoaderTreePrivate::setStorageData(QLoaderStoragePrivate &d_ref)
{
    d.storage.d_ptr = &d_ref;
}
