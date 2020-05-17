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

#include "tst_render.h"
#include "qmlrenderer.h"
#include <QObject>
#include <QTest>
#include <memory>

Render::Render()
{          
    libDir = QDir::currentPath() + "/../lib_output";
    refDir = QDir::currentPath() + "/../reference_output";
}

Render::~Render()
{

}

void Render::test_case1()
{
    int fps = 25;
    int duration = 6;
    int totalFrames = fps*duration;
    QmlRenderer *m_renderer = new QmlRenderer("/home/akhilkg/anim.qml", fps, duration);
    //int total = 25*15;
    int i;
    for(i=0; i<totalFrames; i++) {
        QImage img = m_renderer->render(720, 596, QImage::Format_ARGB32, i);
        QString number = QString::number(i);
        bool ok = img.save("/home/akhilkg/out/frame" + number + ".jpg");
        if(!ok)
            qDebug() << " NOT OKAY MAN \n";
    }

    qDebug() << " FINAL VALUES == " << i << " " << totalFrames;
}

QTEST_MAIN(Render)
