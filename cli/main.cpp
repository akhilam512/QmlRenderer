/*
Copyright (C) 2019  Akhil K Gangadharan <helloimakhil@gmail.com>
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "qmlrender.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QProcess>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("QmlRenderer");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption  file( "i" , QCoreApplication::translate("main", "Set input file" ), "file", "");
    parser.addOption(file);

    QString outputName = "output_frame";

    QCommandLineOption  odir(QStringList() << "o" << "outdir", QCoreApplication::translate("main", "Set output directory" ), "outputdir", "");
    parser.addOption(odir);

    QCommandLineOption  format(QStringList() << "f" << "outformat", QCoreApplication::translate("main", "Set output format" ),"format");
    format.setDefaultValue("jpg");
    parser.addOption(format);

    QCommandLineOption  size(QStringList() << "s" << "size", QCoreApplication::translate("main", "Set frame size of output, enter in format AxB, eg: 1280x720" ),"size");
    size.setDefaultValue("1280x720");
    parser.addOption(size);

    QCommandLineOption  devicePRatio(QStringList() << "D" << "devicepixelratio", QCoreApplication::translate("main", "Set device pixel ratio value" ),"dpr");
    devicePRatio.setDefaultValue("1");
    parser.addOption(devicePRatio);

    QCommandLineOption  duration(QStringList() << "d" << "duration", QCoreApplication::translate("main", "Set duration of output video in milli seconds" ), "duration");
    duration.setDefaultValue("1000");
    parser.addOption(duration);

    QCommandLineOption  fps("F", QCoreApplication::translate("main", "Set FPS of output video" ), "fps");
    fps.setDefaultValue("25");
    parser.addOption(fps);

    QCommandLineOption  singleframe(QStringList() << "S" << "singleframe", QCoreApplication::translate("main", "Set to true for a single frame render, by default = false" ), "checkIfSingleFrame");
    singleframe.setDefaultValue("false");
    parser.addOption(singleframe);

    QCommandLineOption  frametime(QStringList() << "t" << "frametime", QCoreApplication::translate("main", "If single frame = true, set which frame, i.e. frame time" ), "frameTime");
    frametime.setDefaultValue("1000");
    parser.addOption(frametime);

    parser.process(app);

    if(parser.value(file).isNull() || parser.value(odir).isNull()) {
        qDebug() << "Missing arguments";
        return 1;
    }

    QStringList frameSizeList = parser.value(size).split("x");                      // for 1280x720, frameSizeList will contain ["1280", "720"]
    QSize frameSize(frameSizeList.at(0).toInt(), frameSizeList.at(1).toInt());

    bool ifSingleFrame = parser.value(singleframe)=="true"? true:false ;

    QmlRender w;
    w.renderer->initialiseRenderParams(QUrl(parser.value(file)), outputName, parser.value(odir), parser.value(format), frameSize, parser.value(devicePRatio).toLongLong(), parser.value(duration).toInt(), parser.value(fps).toInt(), ifSingleFrame , parser.value(frametime).toLongLong());
    w.renderer->prepareRenderer();
    w.renderer->renderQml();
    return app.exec();
}
