/*
 *    Copyright 2012, 2013 Thomas Schöps
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

#include <cstdio>

#include <QtNumeric>
#include <QDebug>
#include <QLatin1String>
#include <QString>
#include <QStringRef>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>

#include "util/xml_stream_util.h"


namespace OpenOrienteering {

void Matrix::save(QXmlStreamWriter& xml, const QString& role) const
{
	XmlElementWriter matrix{xml, QLatin1String("matrix")};
	matrix.writeAttribute(QLatin1String("role"), role);
	matrix.writeAttribute(QLatin1String("n"), n);
	matrix.writeAttribute(QLatin1String("m"), m);
	int count = n * m;
	for (int i=0; i<count; i++)
	{
		XmlElementWriter element{xml, QLatin1String("element")};
		element.writeAttribute(QLatin1String("value"), d[i]);
	}
}

void Matrix::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == QLatin1String("matrix"));
	
	XmlElementReader matrix{xml};
	int new_n = qMax(0, matrix.attribute<int>(QLatin1String("n")));
	int new_m = qMax(0, matrix.attribute<int>(QLatin1String("m")));
	setSize(new_n, new_m);
	int count = n*m;
	int i = 0;
	while (xml.readNextStartElement())
	{
		if (i < count && xml.name() == QLatin1String("element"))
		{
			d[i] = xml.attributes().value(QLatin1String("value")).toDouble();
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

double Matrix::determinant() const
{
	Matrix a = Matrix(*this);
	
	double result = 1;
	for (int i = 1; i < n; ++i)
	{
		// Pivot search
		if (a.get(i - 1, i - 1) <= 0.001)
		{
			double highest = a.get(i - 1, i - 1);
			int highest_pos = i - 1;
			for (int k = i; k < n; ++k)
			{
				double v = a.get(k, i - 1);
				if (v > highest)
				{
					highest = v;
					highest_pos = k;
				}
			}
			if (highest == 0)
				return 0;
			if (i - 1 != highest_pos)
			{
				a.swapRows(i - 1, highest_pos);
				result = -result;
			}
		}
		
		result *= a.get(i-1, i-1);
		
		for (int k = i; k < n; ++k)
		{
			double factor = -a.get(k, i - 1) / a.get(i - 1, i - 1);
			for (int j = i; j < m; ++j)
				a.set(k, j, a.get(k, j) + factor * a.get(i - 1, j));
		}
	}
	result *= a.get(n-1, n-1);
	
	if (qIsNaN(result))
	{
		print();
		Q_ASSERT(false);
	}
	
	return result;
}

bool Matrix::invert(Matrix& out) const
{
	Matrix a = Matrix(*this);
	out.setSize(n, m);
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < m; ++j)
			out.set(i, j, (i == j) ? 1 : 0);
	
	for (int i = 1; i < n; ++i)
	{
		// Pivot search
		if (true) //a.get(i - 1, i - 1) <= 0.001)
		{
			double highest = qAbs(a.get(i - 1, i - 1));
			int highest_pos = i - 1;
			for (int k = i; k < n; ++k)
			{
				double v = qAbs(a.get(k, i - 1));
				if (v > highest)
				{
					highest = v;
					highest_pos = k;
				}
			}
			if (highest == 0)
				return false;
			if (i - 1 != highest_pos)
			{
				a.swapRows(i - 1, highest_pos);
				out.swapRows(i - 1, highest_pos);
			}
		}
		
		for (int k = i; k < n; ++k)
		{
			double factor = -a.get(k, i - 1) / a.get(i - 1, i - 1);
			for (int j = 0; j < m; ++j)
			{
				a.set(k, j, a.get(k, j) + factor * a.get(i - 1, j));
				out.set(k, j, out.get(k, j) + factor * out.get(i - 1, j));
			}
		}
	}
	for (int i = n - 2; i >= 0; --i)
	{
		for (int k = i; k >= 0; --k)
		{
			double factor = -a.get(k, i + 1) / a.get(i + 1, i + 1);
			for (int j = 0; j < m; ++j)
			{
				a.set(k, j, a.get(k, j) + factor * a.get(i + 1, j));
				out.set(k, j, out.get(k, j) + factor * out.get(i + 1, j));
			}
		}
	}
	for (int i = 0; i < n; ++i)
	{
		double factor = 1 / a.get(i, i);
		for (int j = 0; j < m; ++j)
			out.set(i, j, out.get(i, j) * factor);
	}
	
	return true;
}

void Matrix::print() const
{
	for (int i = 0; i < n; ++i)
	{
		printf("( ");
		for (int j = 0; j < m; ++j)
		{
			printf("%f ", get(i, j));
		}
		printf(")\n");
	}
	printf("\n");
}


}  // namespace OpenOrienteering
