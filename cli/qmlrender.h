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

#ifndef QMLRENDER_H
#define QMLRENDER_H

#include "qmlrenderer.h"
#include <memory>

class QmlRender : public QObject
{
    Q_OBJECT

public:
    explicit QmlRender(QString filename, QObject *parent = nullptr);
    ~QmlRender();

    std::unique_ptr<QmlRenderer> renderer;
private:
    QString m_filename;

};

#endif // QMLRENDER_H
