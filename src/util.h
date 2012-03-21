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


#ifndef UTIL_H
#define UTIL_H

#include "map_coord.h"

#include <QDoubleValidator>
#include <QRectF>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

#define BEZIER_KAPPA 0.5522847498

/// Double validator for line edit widgets
class DoubleValidator : public QDoubleValidator
{
public:
	DoubleValidator(double bottom, double top = 10e10, QObject* parent = NULL, int decimals = 20);
	
	virtual State validate(QString& input, int& pos) const;
};

/// Enlarges the rect to include the given point
void rectInclude(QRectF& rect, MapCoordF point); // does not work if rect is invalid
void rectInclude(QRectF& rect, QPointF point); // does not work if rect is invalid
void rectIncludeSafe(QRectF& rect, QPointF point); // checks if rect is invalid
/// Enlarges the rect to include the given rect
void rectInclude(QRectF& rect, const QRectF& other_rect); // does not work if rect is invalid
void rectIncludeSafe(QRectF& rect, const QRectF& other_rect); // checks if rect is invalid

/// Helper functions to save a string to a file and load it again
void saveString(QFile* file, const QString& str);
void loadString(QFile* file, QString& str);

#endif
