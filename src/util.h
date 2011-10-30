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


#ifndef UTIL_H
#define UTIL_H

#include <QDoubleValidator>
#include <QRectF>

/// Double validator for line edit widgets
class DoubleValidator : public QDoubleValidator
{
public:
	DoubleValidator(double bottom, double top = 10e10, QObject* parent = NULL, int decimals = 20);
	
	virtual State validate(QString& input, int& pos) const;
};

void rectInclude(QRectF& rect, QPointF point);

#endif
