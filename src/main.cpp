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

#include <QApplication>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QLoaderTree>
#include <QMessageBox>

namespace {

QCoreApplication *createApplication(int &argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (!qstrcmp(argv[i], "--no-gui"))
        {
            return new QCoreApplication(argc, argv);
        }
    }
    return new QApplication(argc, argv);
}

}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setApplicationName("Qt Loader");

    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

#ifdef Q_OS_LINUX
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

    parser.addPositionalArgument("file", "Set .qt file");
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
    QLoaderTree::Error error = loaderTree.load();
    if (error.status)
    {
        if (error.status == QLoaderTree::AccessError)
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
        QString message = fileName + ": " + QString::number(error.line) + ": " +
                          QVariant::fromValue(error.status).toString().toLower();

        message.insert(message.size() - QString("error").size(), ' ');
        message += ": " + error.message;

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
