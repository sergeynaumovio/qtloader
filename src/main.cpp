// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadertree.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QMessageBox>

using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setApplicationName(u"Qt Loader"_s);

#ifdef Q_OS_WINDOWS
    QApplication::setStyle("fusion");
#endif

    QScopedPointer<QCoreApplication> app([&]() -> QCoreApplication *
    {
        for (int i = 1; i < argc; ++i)
            if (!qstrcmp(argv[i], "--no-gui"))
                return new QCoreApplication(argc, argv);

        return new QApplication(argc, argv);
    }());

#ifndef Q_OS_WINDOWS
    app->addLibraryPath(u"/usr/lib"_s);
    app->addLibraryPath(u"/usr/local/lib"_s);
    const QStringList ld = qEnvironmentVariable("LD_LIBRARY_PATH").split(u':');
    for (const QString &path : ld)
        app->addLibraryPath(path);
#endif

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption({u"s"_s, u"section"_s}, u"Root section."_s));
    parser.addOption(QCommandLineOption(u"no-gui"_s, u"Start console application."_s));

    parser.addPositionalArgument(u"file"_s, u"Set .qt file."_s);
    parser.process(*app);
    QString fileName;

    bool coreApp = !qobject_cast<QApplication*>(app.data());
    QStringList arguments = parser.positionalArguments();
    if (!arguments.size() || !arguments.first().size())
    {
        if (coreApp)
        {
            qInfo().noquote() << "Argument <file> is empty.";
            return -1;
         }
        else
        {
            fileName = QFileDialog::getOpenFileName(nullptr,
                                                    u"Open Qt File"_s,
                                                    u""_s,
                                                    u"Qt File (*.qt)"_s);

            if (!fileName.size())
                return -1;
        }
    }
    else
        fileName = arguments.first();

    QLoaderTree loaderTree(fileName);
    if (QLoaderError error = loaderTree.load())
    {
        if (error.status == QLoaderError::Access)
        {
            if (coreApp)
            {
                qInfo().noquote() << "File not found" << fileName;
                return -1;
            }
            else
            {
                QString messsage = u"File not found \""_s + fileName + u"\""_s;
                return QMessageBox::warning(nullptr, u"Qt Loader"_s,
                                            QDir::toNativeSeparators(messsage),
                                            QMessageBox::Close);
            }
        }
        QString message = fileName + u':' + QString::number(error.line) + u": "_s +
                          QVariant::fromValue(error.status).toString().toLower() +
                          u" error: "_s + error.message;;

        if (coreApp)
        {
            qInfo().noquote() << message;
            return -1;
        }
        else
            return QMessageBox::critical(nullptr, u"Qt Loader"_s,
                                         QDir::toNativeSeparators(message),
                                         QMessageBox::Close);
    }

    return app->exec();
}
