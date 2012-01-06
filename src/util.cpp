/*
 *    Copyright 2011 Thomas Schöps
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


#include "util.h"

#include <QFile>

DoubleValidator::DoubleValidator(double bottom, double top, QObject* parent, int decimals) : QDoubleValidator(bottom, top, decimals, parent)
{
	// Don't cause confusion, accept only English formatting
	setLocale(QLocale(QLocale::English));
}
QValidator::State DoubleValidator::validate(QString& input, int& pos) const
{
	// Transform thousands group separators into decimal points to avoid confusion
	input.replace(',', '.');
	
	return QDoubleValidator::validate(input, pos);
}

void rectInclude(QRectF& rect, QPointF point)
{
	if (point.x() < rect.left())
		rect.setLeft(point.x());
	else if (point.x() > rect.right())
		rect.setRight(point.x());
	
	if (point.y() < rect.top())
		rect.setTop(point.y());
	else if (point.y() > rect.bottom())
		rect.setBottom(point.y());
}
void rectInclude(QRectF& rect, QRectF other_rect)
{
	if (other_rect.left() < rect.left())
		rect.setLeft(other_rect.left());
	if (other_rect.right() > rect.right())
		rect.setRight(other_rect.right());
	
	if (other_rect.top() < rect.top())
		rect.setTop(other_rect.top());
	if (other_rect.bottom() > rect.bottom())
		rect.setBottom(other_rect.bottom());
}

void saveString(QFile* file, const QString& str)
{
	int length = str.length();
	
	file->write((const char*)&length, sizeof(int));
	file->write((const char*)str.constData(), length * sizeof(QChar));
}
void loadString(QFile* file, QString& str)
{
	int length;
	
	file->read((char*)&length, sizeof(int));
	if (length > 0)
	{
		str.resize(length);
		file->read((char*)str.data(), length * sizeof(QChar));
	}
	else
		str = "";
}
