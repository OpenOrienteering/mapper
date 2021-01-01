/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <QObject>
#include <QString>

#include "libvectorizer/Polygons.h"

class PolygonTest : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	
	void testJoins_data();
	void testJoins();

private:
	void saveResults(const cove::PolygonList& polys,
	                 const QString& filename) const;
	void compareResults(const cove::PolygonList& polys,
	                    const QString& filename) const;
};
