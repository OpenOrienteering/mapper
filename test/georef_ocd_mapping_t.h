/*
*    Copyright 2017 Libor Pecháček
*
*    This file is part of OpenOrienteering.
*
*    OpenOrienteering is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    OpenOrienteering is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with OpenOrienteering.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPENORIENTEERING_GEOREF_OCD_MAPPING_TEST_H
#define OPENORIENTEERING_GEOREF_OCD_MAPPING_TEST_H

#include <QObject>

class GeoreferenceMappingTest : public QObject
{
    Q_OBJECT

private slots:

    void initTestCase();

    void testOcdToMapper_data();
    void testOcdToMapper();
};

#endif // OPENORIENTEERING_GEOREF_OCD_MAPPING_TEST_H
