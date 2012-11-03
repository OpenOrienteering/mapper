/*
 *    Copyright 2012 Thomas Schöps
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


#include "matrix.h"

#include <QDebug>
#include <QIODevice>
#include <QXmlStreamWriter>

void Matrix::save(QIODevice* file)
{
	file->write((const char*)&n, sizeof(int));
	file->write((const char*)&m, sizeof(int));
	file->write((const char*)d, n*m * sizeof(double));
}

void Matrix::load(QIODevice* file)
{
	int new_n, new_m;
	file->read((char*)&new_n, sizeof(int));
	file->read((char*)&new_m, sizeof(int));
	
	setSize(new_n, new_m);
	file->read((char*)d, n*m * sizeof(double));
}

void Matrix::save(QXmlStreamWriter& xml, const QString role)
{
	xml.writeStartElement("matrix");
	xml.writeAttribute("role", role);
	xml.writeAttribute("n", QString::number(n));
	xml.writeAttribute("m", QString::number(m));
	int count = n * m;
	for (int i=0; i<count; i++)
	{
		xml.writeStartElement("element");
		xml.writeAttribute("value", QString::number(d[i]));
		xml.writeEndElement();
	}
	xml.writeEndElement(/*matrix*/);
}

void Matrix::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == "matrix");
	
	int new_n = qMax(0, xml.attributes().value("n").toString().toInt());
	int new_m = qMax(0, xml.attributes().value("m").toString().toInt());
	setSize(new_n, new_m);
	int count = n*m;
	int i = 0;
	while (xml.readNextStartElement())
	{
		if (i < count && xml.name() == "element")
		{
			d[i] = xml.attributes().value("value").toString().toDouble();
			i++;
		}
		xml.skipCurrentElement();
	}
	
	if (i < count)
	{
		qDebug() << "Too few elements for a" << new_n << "x" << new_m
		         << "matrix at line" << xml.lineNumber();
	}
}
