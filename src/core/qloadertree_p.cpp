// Copyright (C) 2025 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadertree_p.h"
#include "qloadermarkupeditor.h"
#include "qloadertree.h"
#include "qloaderplugininterface.h"
#include "qloadersaveinterface.h"
#include "qloadersettings.h"
#include "qloadershell.h"
#include "qloadershellcd.h"
#include "qloadershellexit.h"
#include "qloadershellsave.h"
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

using namespace Qt::Literals::StringLiterals;

static QStringView objectName(QStringView section)
{
    return section.sliced(section.lastIndexOf(u'/', -2) + 1);
}

static QStringView parentSection(QStringView section)
{
    int splitIndex = section.lastIndexOf(u'/', -2);

    return (splitIndex == -1 ? QStringView{} : section.first(splitIndex));
}

class QLoaderTreeSection
{
    QLoaderTreeSection(QStringView section)
    :   section(section)
    {
        if ((parent.section = parentSection(section)).isEmpty())
            return;

        valid = true;
    }

public:
    bool valid{};

    struct
    {
        QStringView section;
        QLoaderSettings *settings{};

    } parent;

    QStringView section;
    QLoaderSettings *settings{};
    QObject *object{};

    QLoaderTreeSection(QStringView section, QLoaderTreePrivate *d)
    :   QLoaderTreeSection(section)
    {
        if (!valid)
            return;

        d->mutex.lock();
        if (!d->hash.settings.sections.contains(parent.section))
            valid = false;
        else
        {
            valid = true;
            parent.settings = d->hash.settings.sections.value(parent.section);
            settings = d->hash.settings.sections.value(section);
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

    QLoaderError actionError() const;
    QString actionPath () const { return u'[' + src.section.toString()+ u"] -> ["_s + dst.section.toString()+ u']'; }

public:
    QLoaderTreeSection src;
    QLoaderTreeSection dst;

    QLoaderTreeSectionAction(QStringView srcPath,
                             QStringView dstPath,
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
            return {.status = QLoaderError::Object, .message = u"tree not loaded"_s};

        if (!src.valid || !dst.valid)
            return {.status = QLoaderError::Design, .message = u"section not valid"_s};

        if (QLoaderError err = actionError())
        {
            emit d_ptr->q_ptr->errorChanged(src.object, err.message);
            return err;
        }

        return {};
    }

    static QString section(const QStringList &itemSection,
                           const QStringList &srcSection,
                           const QStringList &dstSection)
    {
        int size = itemSection.size() + (dstSection.size() - srcSection.size());
        QStringList retSection(size);
        std::copy_n(dstSection.begin(), dstSection.size(), retSection.begin());
        std::copy_n(itemSection.begin() + srcSection.size(),
                    itemSection.size() - srcSection.size(),
                    retSection.begin() + dstSection.size());

        return retSection.join(u'/');
    }
};

template<>
QLoaderError QLoaderTreeSectionAction<Copy>::actionError() const
{
    if (!d_ptr->hash.data[src.settings].settings.front()->isCopyable(dst.section))
        return {.status = QLoaderError::Object, .message = actionPath() + u" : copy operation not allowed"_s};

    return {};
}

template<>
QLoaderError QLoaderTreeSectionAction<Move>::actionError() const
{
    if (!d_ptr->hash.data[src.settings].settings.front()->isMovable(dst.section))
        return {.status = QLoaderError::Object, .message = actionPath() + u" : move operation not allowed"_s};

    return {};
}

class KeyValueParser
{
    const QRegularExpression className{u"^[A-Z]+[a-z,0-9]*"_s};

public:
    bool matchSectionName(const QString &line, QString &section) const
    {
        if (line.size() > 2 &&
            line.front() == u'[' &&
            line.back() == u']' &&
            line.indexOf(u'[', 1) == -1 &&
            line.lastIndexOf(u']', -2) == -1)
        {
            section = line.sliced(1, line.size() - 2);
            return true;
        }

        return false;
    }

    bool matchKeyValue(const QString &line, QString &key, QString &value) const
    {
        int splitIndex = line.indexOf(u'=');

        if (splitIndex > 0)
        {
            QStringView view(line);

            if ((view = view.first(splitIndex).trimmed()).size())
            {
                key = view.toString();
                value = QStringView(line).sliced(splitIndex + 1).trimmed().toString();
                return true;
            }
        }

        return false;
    }

    bool matchClassName(const char *name, QString &libraryName) const
    {
        QRegularExpressionMatch match;
        if ((match = className.match(QString::fromLocal8Bit(name))).hasMatch())
        {
            libraryName += match.capturedView();
            return true;
        }

        return false;
    }
};

class StringVariantConverter
{
    const QRegularExpression bytearray{u"^QByteArray\\s*\\(\\s*(.*)\\)"_s};

    const QRegularExpression color_rgb{u"^QColor\\s*\\(\\s*(?<r>\\d+)\\s*\\,"
                                       u"\\s*(?<g>\\d+)\\s*\\,"
                                       u"\\s*(?<b>\\d+)\\s*\\)"_s};

    const QRegularExpression color_rgba{u"^QColor\\s*\\(\\s*(?<r>\\d+)\\s*\\,"
                                        u"\\s*(?<g>\\d+)\\s*\\,"
                                        u"\\s*(?<b>\\d+)\\s*\\,"
                                        u"\\s*(?<a>\\d+)\\s*\\)"_s};

    const QRegularExpression size{u"^QSize\\s*\\(\\s*(?<width>\\d+)\\s*\\,\\s*(?<height>\\d+)\\s*\\)"_s};

public:
    QVariant fromString(const QString &value) const
    {
        if (value.at(0) != u'Q')
            return value;

        QStringView view(QStringView(value).sliced(1));
        QRegularExpressionMatch match;

        if (view.startsWith("ByteArray"_L1) && (match = bytearray.match(value)).hasMatch())
            return QVariant::fromValue(QByteArray::fromBase64(match.capturedView(1).toLocal8Bit()));

        if (view.startsWith("Color"_L1))
        {
            if ((match = color_rgb.match(value)).hasMatch())
                return QColor(match.capturedView(u"r"_s).toInt(),
                              match.capturedView(u"g"_s).toInt(),
                              match.capturedView(u"b"_s).toInt());

            if ((match = color_rgba.match(value)).hasMatch())
                return QColor(match.capturedView(u"r"_s).toInt(),
                              match.capturedView(u"g"_s).toInt(),
                              match.capturedView(u"b"_s).toInt(),
                              match.capturedView(u"a"_s).toInt());
        }

        if (view.startsWith("Size"_L1) && (match = size.match(value)).hasMatch())
            return QSize(match.capturedView(u"width"_s).toInt(), match.capturedView(u"height"_s).toInt());

        return value;
    }

    QString fromVariant(const QVariant &variant) const
    {
        QMetaType::Type type = static_cast<QMetaType::Type>(variant.metaType().id());

        if (type == QMetaType::QByteArray)
            return QString(u"QByteArray("_s + QLatin1StringView(variant.toByteArray().toBase64()) + u')');

        if (type == QMetaType::QSize)
            return QString(u"QSize("_s + QString::number(variant.toSize().width()) + u", "_s
                                       + QString::number(variant.toSize().height()) + u')');

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

class QLoaderTreePrivateData
{
public:
    const QString libraryPrefix{u"Qt"_s + QString::number(QT_VERSION_MAJOR)};
    QHash<QString, QLoaderPluginInterface *> plugins;

    SettingsObject root;
    SettingsObject shell;

    KeyValueParser parser;
    StringVariantConverter converter;
    QMutex loading;
    QString shebang;
    QList<QLoaderSettings *> copied;
    Saving saving;

    ~QLoaderTreePrivateData()
    {
        if (shell.object) delete shell.object;
    }
};

QLoaderTreePrivate::QLoaderTreePrivate(const QString &fileName, QLoaderTree *q)
:   d(*new (&d_storage) QLoaderTreePrivateData),
    q_ptr(q),
    file(new QFile(fileName, q))
{
    static_assert(sizeof(QLoaderTreePrivateData) == sizeof(d_storage));
    static_assert(alignof(QLoaderTreePrivateData) == 8);
}

QLoaderTreePrivate::~QLoaderTreePrivate()
{
    d.~QLoaderTreePrivateData();
    qDeleteAll(hash.settings.sections);
}

QObject *QLoaderTreePrivate::builtin(QLoaderSettings *settings, QObject *parent)
{
    const char *shortName = settings->className() + std::char_traits<char>::length("QLoader");

    if (!strcmp(shortName, "ShellCd"))
    {
        if (QLoaderShell *shell = qobject_cast<QLoaderShell *>(parent))
            return new QLoaderShellCd(settings, shell);

        return parent;
     }

    if (!strcmp(shortName, "ShellExit"))
    {
        if (QLoaderShell *shell = qobject_cast<QLoaderShell *>(parent))
            return new QLoaderShellExit(settings, shell);

        return parent;
     }

    if (!strcmp(shortName, "ShellSave"))
    {
        if (QLoaderShell *shell = qobject_cast<QLoaderShell *>(parent))
            return new QLoaderShellSave(settings, shell);

        return parent;
     }

    if (!strcmp(shortName, "Shell"))
    {
        if (!d.shell.object)
            return d.shell.object = new QLoaderShell(settings);

        return nullptr;
     }

    bool coreApp = !qobject_cast<QApplication *>(QCoreApplication::instance());
    QWidget *widget = qobject_cast<QWidget *>(parent);

    if (!strcmp(shortName, "Terminal"))
    {
        if (coreApp)
            return nullptr;

        if (!parent || (parent && widget))
            return new QLoaderTerminal(settings, widget);

        return parent;
    }

    if (!strcmp(shortName, "MarkupEditor"))
    {
        if (coreApp)
            return nullptr;

        if (!parent || (parent && widget))
            return new QLoaderMarkupEditor(settings, widget);

        return parent;
    }

    return q_ptr;
}

QObject *QLoaderTreePrivate::external(QLoaderError &error,
                                      QLoaderSettings *settings,
                                      QObject *parent)
{
    QString libraryName = d.libraryPrefix;

    mutex.lock();
    QString pluginName = hash.data[settings].pluginName;
    mutex.unlock();

    if (pluginName.size())
        libraryName += pluginName;

    if (pluginName.size() || d.parser.matchClassName(settings->className(), libraryName))
    {
        QLoaderPluginInterface *plugin = d.plugins.value(libraryName);

        if (plugin)
            return plugin->object(settings, parent);

        QPluginLoader *loader = new QPluginLoader(libraryName, q_ptr);
        if (!loader->instance())
        {
            mutex.lock();
            error.line = hash.data[settings].sectionLine;
            error.status = QLoaderError::Plugin;
            error.message = u"library \""_s + libraryName + u"\" not loaded"_s;
            mutex.unlock();
            return nullptr;
        }

        if (!(plugin = qobject_cast<QLoaderPluginInterface *>(loader->instance())))
        {
            mutex.lock();
            error.line = hash.data[settings].sectionLine;
            error.status = QLoaderError::Plugin;
            error.message = u"interface not valid"_s;
            mutex.unlock();
            return nullptr;
        }

        d.plugins[libraryName] = plugin;

        return plugin->object(settings, parent);
    }

    mutex.lock();
    error.line = hash.data[settings].sectionLine;
    error.status = QLoaderError::Plugin;
    error.message = u"class name \""_s + QLatin1StringView(hash.data[settings].className) + u"\" not valid"_s;

    mutex.unlock();
    return nullptr;
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
    qDebug().noquote().nospace() << u'[' << hash.data[settings].section << u']';
    qDebug().noquote() << u"class ="_s << hash.data[settings].className;

    if (hash.data[settings].pluginName.size())
        qDebug().noquote() << u"plugin ="_s << hash.data[settings].pluginName;

    QMapIterator<QString, QLoaderProperty> i(hash.data[settings].properties);
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
    if (d.shebang.size())
        qDebug().noquote() << d.shebang;

    dumpRecursive(settings);
}

QLoaderError QLoaderTreePrivate::loadRecursive(QLoaderSettings *settings, QObject *parent)
{
    QLoaderError error;

    mutex.lock();
    const char *itemClassName = hash.data[settings].className;
    mutex.unlock();

    QObject *object;
    if (!strncmp(itemClassName, "QLoader", 7))
        object = builtin(settings, parent);
    else
        object = external(error, settings, parent);

    if (!object)
       return error;

    mutex.lock();
    int itemSectionSize = hash.data[settings].section.count(u'/') + 1;
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
            error.message = u"parent object not valid"_s;
        }
        else if (object == q_ptr)
        {
            error.message = u"class \""_s + QLatin1StringView(itemClassName) + u"\" not found"_s;
        }
        else if (object != d.shell.object && !object->parent())
        {
            error.message = u"parent object not set"_s;
            delete object;
        }

        return error;
    }

    if (!d.root.object && itemSectionSize == 1)
        d.root.object = object;

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
    QList<QLoaderSettings *> children = hash.data[settings].children;
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

    QString sectionString = QLoaderTreeSectionAction<Move>::section(item.section.split(u'/'),
                                                                    src.section.toString().split(u'/'),
                                                                    dst.section.toString().split(u'/'));

    hash.settings.sections.remove(item.section);
    hash.settings.sections[sectionString] = settings;
    item.section = std::move(sectionString);

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
        emit q_ptr->errorChanged(shell, u"shell not loaded"_s);
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
                emit q_ptr->errorChanged(object, u"constructor not invokable"_s);
        }
        else
            emit q_ptr->errorChanged(object, u"interface not found"_s);
    }

    return shell;
}

QLoaderError QLoaderTreePrivate::readSettings()
{
    QLoaderError error;
    if (!file->open(QIODevice::ReadOnly | QIODevice::Text))
    {
        error.status = QLoaderError::Access;
        error.message = u"read error"_s;
        return error;
    }

    QTextStream in(file);
    QString line;
    line.reserve(10240);
    int currentLine{};

    const QLatin1StringView shebang("#!"_L1);
    const QChar comment('#'_L1);

    QLoaderSettings *settings{};
    QLoaderSettingsData item;

    while (!in.atEnd())
    {
        in.readLineInto(&line);
        ++currentLine;

        if (currentLine == 1 && line.startsWith(shebang))
        {
            d.shebang = line;
            continue;
        }

        if (line.isEmpty())
            continue;

        if (line.startsWith(comment))
            continue;

        QString section;
        if (d.parser.matchSectionName(line, section))
        {
            QStringView name = objectName(section);
            int level = section.count(u'/') + 1;

            if (settings)
            {
                if (item.className.isEmpty())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Design;
                    error.message = u"class name not set"_s;
                    break;
                }
                hash.data[settings] = std::move(item);
            }

            settings = new QLoaderSettings(*this);

            bool valid{};

            if (!hash.settings.sections.contains(section))
            {
                hash.settings.sections[section] = settings;

                if (level == 1 && name.size())
                {
                    if (!d.root.settings)
                        valid = true;
                    else
                        error.message = u"root object already set"_s;
                }
                else if (d.root.settings && level > 1 && name.size())
                {
                    QStringView parent = parentSection(section);

                    if (hash.settings.sections.contains(parent))
                    {
                        valid = true;
                        item.parent = hash.settings.sections.value(parent);
                        hash.data[item.parent].children.push_back(settings);
                    }
                }
                else
                    error.message = u"section not valid"_s;
            }
            else
            {
                error.message = u"section already set"_s;
                delete settings;
            }

            item.section = std::move(section);
            item.sectionLine = currentLine;
            item.level = level;

            if (!valid)
            {
                error.line = currentLine;
                error.status = QLoaderError::Design;
                if (error.message.isEmpty())
                    error.message = u"section not valid"_s;

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
                error.message = u"section not set"_s;
                break;
            }

            if (key == "class"_L1)
            {
                if (item.className.size())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Format;
                    error.message = u"class already set"_s;
                    break;
                }

                if (!d.root.settings && item.level == 1)
                    d.root.settings = settings;

                item.className = value.toLocal8Bit();
                if (!strncmp(item.className.data(), "Loader", 6))
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Object;
                    error.message = u"class not found"_s;
                    break;
                }

                bool isShell{};
                const char *shortName = item.className.data() + std::char_traits<char>::length("QLoader");
                if ((isShell = !strcmp(shortName, "Shell")))
                {
                    if (item.level > 2)
                    {
                        error.line = item.sectionLine;
                        error.status = QLoaderError::Design;
                        error.message = u"parent object not valid"_s;
                        break;
                    }

                    if (isShell && d.shell.settings)
                    {
                        error.line = item.sectionLine;
                        error.status = QLoaderError::Design;
                        error.message = u"shell object already set"_s;
                        break;
                    }

                    if (isShell)
                        d.shell.settings = settings;
                }
            }
            else if (key == "plugin"_L1)
            {
                if (item.pluginName.size())
                {
                    error.line = item.sectionLine;
                    error.status = QLoaderError::Format;
                    error.message = u"plugin already set"_s;
                    break;
                }

                item.pluginName = value;
            }
            else if (!item.properties.contains(key))
                item.properties[key] = std::move(value);
            else
            {
                error.status = QLoaderError::Design;
                error.message = u"key \""_s + key + u"\" already set"_s;
                break;
            }

            continue;
        }

        if (error.message.isEmpty())
        {
            error.line = currentLine;
            error.status = QLoaderError::Format;
            error.message = u"string not valid"_s;
        }

        break;
    }

    hash.data[settings] = std::move(item);

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
            error.message = u"already loaded"_s;
            break;
        }

        if ((error = readSettings()))
            break;

        if (d.shell.settings && (error = loadRecursive(d.shell.settings, q_ptr)))
            break;

        bool coreApp = !qobject_cast<QApplication *>(QCoreApplication::instance());
        if ((error = loadRecursive(d.root.settings, coreApp ? q_ptr : nullptr)))
            break;

        loaded = true;

    } while (0);

    if (!loaded)
    {
        file->close();
        if (d.root.object) delete d.root.object;
    }

    d.loading.unlock();

    if (loaded)
        emit q_ptr->loaded();

    return error;
}

QLoaderError QLoaderTreePrivate::move(QStringView section, QStringView to)
{
    QLoaderTreeSectionAction<Move> mv(section, to, this);

    if (QLoaderError error = mv.error())
        return error;

    mutex.lock();
    hash.data[mv.src.parent.settings].children.removeOne(mv.src.settings);
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
    QString itemSection = hash.data[settings].section;

    QString section = QLoaderTreeSectionAction<Copy>::section(itemSection.split(u'/'),
                                                              src.section.toString().split(u'/'),
                                                              dst.section.toString().split(u'/'));

    QLoaderSettings *copySettings = new QLoaderSettings(*this);
    d.copied.append(copySettings);
    hash.settings.sections[section] = copySettings;

    QStringView parentSection = ::parentSection(section);

    QLoaderSettings *parentSettings = hash.settings.sections[parentSection];

    hash.data[parentSettings].children.push_back(copySettings);

    hash.data[copySettings].parent = parentSettings;
    hash.data[copySettings].section = std::move(section);
    hash.data[copySettings].className = hash.data[settings].className;
    hash.data[copySettings].pluginName = hash.data[settings].pluginName;
    hash.data[copySettings].properties = hash.data[settings].properties;

    for (QLoaderSettings *child : hash.data[settings].children)
        copyRecursive(child, src, dst);
}

QLoaderError QLoaderTreePrivate::copy(QStringView section, QStringView to)
{
    QLoaderTreeSectionAction<Copy> cp(section, to, this);

    QLoaderError error;
    if ((error = cp.error()))
        return error;

    mutex.lock();
    copyRecursive(cp.src.settings, cp.src, cp.dst);
    QLoaderSettings *settings = hash.settings.sections[cp.dst.section];
    QObject *parent = hash.data[hash.settings.sections[cp.dst.parent.section]].object;
    mutex.unlock();

    error = loadRecursive(settings, parent);

    mutex.lock();
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
    out << "\n[" << item.section << "]\n";
    out << "class = " << item.className << '\n';

    if (item.pluginName.size())
        out << "plugin = " << item.pluginName << '\n';

    QMapIterator<QString, QLoaderProperty> i(item.properties);
    while (i.hasNext())
    {
        i.next();
        out << i.key() << " = " << i.value() << '\n';
    }

    QLoaderSaveInterface *resources = qobject_cast<QLoaderSaveInterface *>(item.object);
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
    QLoaderError error{.status = QLoaderError::Access, .message = u"read-only file"_s};
    if (loaded)
    {
        Saving saving(&d.saving);

        QFileDevice::Permissions permissions = file->permissions();

        QString fileName = file->fileName();
        QString bakFileName = fileName + u".bak"_s;

        QFile ofile(bakFileName);
        if (!ofile.open(QIODevice::WriteOnly))
            return error;

        QTextStream out(&ofile);

        if (d.shebang.size())
            out << d.shebang << '\n';

        saveRecursive(d.root.settings, out);
        ofile.close();

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
        error.message = u"tree not loaded"_s;
    }

    return error;
}

void QLoaderTreePrivate::setProperties(const QLoaderSettingsData &item, QObject *object)
{
    object->setObjectName(objectName(item.section));

    auto value = [&item, this](const QString &key, const QVariant defaultValue = QVariant())
    {
        if (item.properties.contains(key))
            return fromString(item.properties[key]);

        return defaultValue;
    };

    QVariant v;
    if (QAction *action = qobject_cast<QAction *>(object))
    {
        if (!(v = value(u"autoRepeat"_s)).isNull())
            action->setAutoRepeat(v.toBool());

        if (!(v = value(u"checkable"_s)).isNull())
            action->setCheckable(v.toBool());

        if (!(v = value(u"checked"_s)).isNull())
            action->setChecked(v.toBool());

        if (!(v = value(u"enabled"_s)).isNull())
            action->setEnabled(v.toBool());

        if (!(v = value(u"text"_s)).isNull())
            action->setText(v.toString());

        return;
    }

    if (!object->isWidgetType())
        return;

    if (QWidget *widget = static_cast<QWidget *>(object))
    {
        if (!(v = value(u"enabled"_s)).isNull())
            widget->setEnabled(v.toBool());

        if (!(v = value(u"fixedHeight"_s)).isNull())
            widget->setFixedHeight(v.toInt());

        if (!(v = value(u"fixedSize"_s)).isNull())
            widget->setFixedSize(v.toSize());

        if (!(v = value(u"fixedWidth"_s)).isNull())
            widget->setFixedWidth(v.toInt());

        if (!(v = value(u"hidden"_s)).isNull())
            widget->setHidden(v.toBool());

        if (!(v = value(u"maximumHeight"_s)).isNull())
            widget->setMaximumHeight(v.toInt());

        if (!(v = value(u"maximumSize"_s)).isNull())
            widget->setMaximumSize(v.toSize());

        if (!(v = value(u"maximumWidth"_s)).isNull())
            widget->setMaximumWidth(v.toInt());

        if (!(v = value(u"minimumHeight"_s)).isNull())
            widget->setMinimumHeight(v.toInt());

        if (!(v = value(u"minimumSize"_s)).isNull())
            widget->setMinimumSize(v.toSize());

        if (!(v = value(u"minimumWidth"_s)).isNull())
            widget->setMinimumWidth(v.toInt());

        if (!(v = value(u"styleSheet"_s)).isNull())
            widget->setStyleSheet(v.toString());

        if (!(v = value(u"windowTitle"_s)).isNull())
            widget->setWindowTitle (v.toString());

        if (!(v = value(u"visible"_s)).isNull())
            widget->setVisible(v.toBool());
    }

    if (QLabel *label = qobject_cast<QLabel *>(object))
    {
        if (!(v = value(u"text"_s)).isNull())
            label->setText(v.toString());

        return;
    }

    if (QMainWindow *mainwindow = qobject_cast<QMainWindow *>(object))
    {
        if (!(v = value(u"windowTitle"_s)).isNull())
            mainwindow->setWindowTitle(v.toString());

        return;
    }

    if (QMenu *menu = qobject_cast<QMenu *>(object))
    {
        if (!(v = value(u"title"_s)).isNull())
            menu->setTitle(v.toString());

        return;
    }
}
