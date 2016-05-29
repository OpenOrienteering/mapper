/**
 * This file is part of OpenOrienteering.
 *
 * This is a modified version of tst_qglobal.cpp from the Qt Toolkit 5.7.
 * You can redistribute it and/or modify it under the terms of
 * the GNU General Public License, version 3, as published by
 * the Free Software Foundation.
 *
 * OpenOrienteering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>
 *
 * Changes:
 * 2016-03-25 Kai Pastor <dg0yt@darc.de>
 * - Adjustment of legal information
 * - Reduction to qOverload related tests
 * - Adaption to our testing framework
 */
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
****************************************************************************/

#ifndef OPENORIENTEERING_TST_QGLOBAL_H
#define OPENORIENTEERING_TST_QGLOBAL_H

#include <QtTest/QtTest>

class tst_QGlobal: public QObject
{
    Q_OBJECT

private slots:
    void testqOverload();
};

#endif // OPENORIENTEERING_TST_QGLOBAL_H
