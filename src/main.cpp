// Copyright (C) 2022 Sergey Naumov <sergey@naumov.io>
// SPDX-License-Identifier: 0BSD

#include "qloadertree.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setApplicationName("Qt Loader");

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
    app->addLibraryPath("/usr/lib");
    app->addLibraryPath("/usr/local/lib");
    const QStringList ld = qEnvironmentVariable("LD_LIBRARY_PATH").split(':');
    for (const QString &path : ld)
        app->addLibraryPath(path);
#endif

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption({"s", "section"}, "Root section."));
    parser.addOption(QCommandLineOption("no-gui", "Start console application."));

    parser.addPositionalArgument("file", "Set .qt file.");
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
                                                    "Open Qt File",
                                                    "",
                                                    "Qt File (*.qt)");

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
                QString messsage = "File not found \"" + fileName + "\"";
                return QMessageBox::warning(nullptr, "Qt Loader",
                                            QDir::toNativeSeparators(messsage),
                                            QMessageBox::Close);
            }
        }
        QString message = fileName + ":" + QString::number(error.line) + ": " +
                          QVariant::fromValue(error.status).toString().toLower() +
                          " error: " + error.message;;

        if (coreApp)
        {
            qInfo().noquote() << message;
            return -1;
        }
        else
            return QMessageBox::critical(nullptr, "Qt Loader",
                                         QDir::toNativeSeparators(message),
                                         QMessageBox::Close);
    }

    return app->exec();
}
