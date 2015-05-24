/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014, 2015 Kai Pastor
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


#include "map_grid.h"

#include <limits>

#include <qmath.h>
#include <QPainter>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "georeferencing.h"
#include "../map.h"
#include "../core/map_coord.h"
#include "../util.h"

struct ProcessLine
{
	QPainter* painter;
	void processLine(QPointF a, QPointF b);
};

void ProcessLine::processLine(QPointF a, QPointF b)
{
	painter->drawLine(a, b);
}

// ### MapGrid ###

MapGrid::MapGrid()
{
	snapping_enabled = true;
	color = qRgba(100, 100, 100, 128);
	display = AllLines;
	
	alignment = MagneticNorth;
	additional_rotation = 0;
	
	unit = MetersInTerrain;
	horz_spacing = 500;
	vert_spacing = 500;
	horz_offset = 0;
	vert_offset = 0;
}

#ifndef NO_NATIVE_FILE_FORMAT

const MapGrid& MapGrid::load(QIODevice* file, int version)
{
	Q_UNUSED(version);
	file->read((char*)&snapping_enabled, sizeof(bool));
	file->read((char*)&color, sizeof(QRgb));
	int temp;
	file->read((char*)&temp, sizeof(int));
	display = (DisplayMode)temp;
	file->read((char*)&temp, sizeof(int));
	alignment = (Alignment)temp;
	file->read((char*)&additional_rotation, sizeof(double));
	file->read((char*)&temp, sizeof(int));
	unit = (Unit)temp;
	file->read((char*)&horz_spacing, sizeof(double));
	file->read((char*)&vert_spacing, sizeof(double));
	file->read((char*)&horz_offset, sizeof(double));
	file->read((char*)&vert_offset, sizeof(double));
	
	return *this;
}

#endif

void MapGrid::save(QXmlStreamWriter& xml) const
{
	xml.writeEmptyElement("grid");
	xml.writeAttribute("color", QColor(color).name());
	xml.writeAttribute("display", QString::number(display));
	xml.writeAttribute("alignment", QString::number(alignment));
	xml.writeAttribute("additional_rotation", QString::number(additional_rotation));
	xml.writeAttribute("unit", QString::number(unit));
	xml.writeAttribute("h_spacing", QString::number(horz_spacing));
	xml.writeAttribute("v_spacing", QString::number(vert_spacing));
	xml.writeAttribute("h_offset", QString::number(horz_offset));
	xml.writeAttribute("v_offset", QString::number(vert_offset));
	xml.writeAttribute("snapping_enabled", snapping_enabled ? "true" : "false");
}

const MapGrid& MapGrid::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == "grid");
	
	QXmlStreamAttributes attributes = xml.attributes();
	color = QColor(attributes.value("color").toString()).rgba();
	display = (MapGrid::DisplayMode) attributes.value("display").toString().toInt();
	alignment = (MapGrid::Alignment) attributes.value("alignment").toString().toInt();
	additional_rotation = attributes.value("additional_rotation").toString().toDouble();
	unit = (MapGrid::Unit) attributes.value("unit").toString().toInt();
	horz_spacing = attributes.value("h_spacing").toString().toDouble();
	vert_spacing = attributes.value("v_spacing").toString().toDouble();
	horz_offset = attributes.value("h_offset").toString().toDouble();
	vert_offset = attributes.value("v_offset").toString().toDouble();
	snapping_enabled = (attributes.value("snapping_enabled") == "true");
	xml.skipCurrentElement();
	
	return *this;
}

void MapGrid::draw(QPainter* painter, QRectF bounding_box, Map* map, bool on_screen) const
{
	double final_horz_spacing, final_vert_spacing;
	double final_horz_offset, final_vert_offset;
	double final_rotation;
	calculateFinalParameters(final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, map);
	
	QPen pen(color);
	if (on_screen)
	{
		// zero-width cosmetic pen (effectively one pixel)
		pen.setWidth(0);
		pen.setCosmetic(true);
	}
	else
	{
		// 0.1 mm wide non-cosmetic pen
		pen.setWidthF(0.1f);
	}
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->setOpacity(qAlpha(color) / 255.0);
	
	ProcessLine process_line;
	process_line.painter = painter;
	
	if (display == AllLines)
		Util::gridOperation<ProcessLine>(bounding_box, final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, process_line);
	else if (display == HorizontalLines)
		Util::hatchingOperation<ProcessLine>(bounding_box, final_vert_spacing, final_vert_offset, final_rotation + M_PI / 2, process_line);
	else // if (display == VeritcalLines)
		Util::hatchingOperation<ProcessLine>(bounding_box, final_horz_spacing, final_horz_offset, final_rotation, process_line);
}

void MapGrid::calculateFinalParameters(double& final_horz_spacing, double& final_vert_spacing, double& final_horz_offset, double& final_vert_offset, double& final_rotation, Map* map) const
{
	double factor = ((unit == MetersInTerrain) ? (1000.0 / map->getScaleDenominator()) : 1);
	final_horz_spacing = factor * horz_spacing;
	final_vert_spacing = factor * vert_spacing;
	final_horz_offset = factor * horz_offset;
	final_vert_offset = factor * vert_offset;
	final_rotation = additional_rotation + M_PI / 2;
	
	const Georeferencing& georeferencing = map->getGeoreferencing();
	if (alignment == GridNorth)
	{
		final_rotation += georeferencing.getGrivation() * M_PI / 180;
		
		// Shift origin to projected coordinates origin
		double prev_horz_offset = MapCoordF::dotProduct(MapCoordF(0, -1).rotated(final_rotation), MapCoordF(georeferencing.getMapRefPoint().xd(), -1 * georeferencing.getMapRefPoint().yd()));
		double target_horz_offset = MapCoordF::dotProduct(MapCoordF(0, -1).rotated(additional_rotation + M_PI / 2), MapCoordF(georeferencing.getProjectedRefPoint().x(), georeferencing.getProjectedRefPoint().y()));
		final_horz_offset -= factor * target_horz_offset - prev_horz_offset;
		
		double prev_vert_offset = MapCoordF::dotProduct(MapCoordF(1, 0).rotated(final_rotation), MapCoordF(georeferencing.getMapRefPoint().xd(), -1 * georeferencing.getMapRefPoint().yd()));
		double target_vert_offset = MapCoordF::dotProduct(MapCoordF(1, 0).rotated(additional_rotation + M_PI / 2), MapCoordF(georeferencing.getProjectedRefPoint().x(), georeferencing.getProjectedRefPoint().y()));
		final_vert_offset += factor * target_vert_offset - prev_vert_offset;
	}
	else if (alignment == TrueNorth)
		final_rotation += georeferencing.getDeclination() * M_PI / 180;
}

MapCoordF MapGrid::getClosestPointOnGrid(MapCoordF position, Map* map) const
{
	double final_horz_spacing, final_vert_spacing;
	double final_horz_offset, final_vert_offset;
	double final_rotation;
	calculateFinalParameters(final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, map);
	
	position.rotate(final_rotation - M_PI / 2);
	return MapCoordF(qRound((position.x() - final_horz_offset) / final_horz_spacing) * final_horz_spacing + final_horz_offset,
					 qRound((position.y() - final_vert_offset) / final_vert_spacing) * final_vert_spacing + final_vert_offset).rotated(-1 * (final_rotation - M_PI / 2));
}



bool operator==(const MapGrid& lhs, const MapGrid& rhs)
{
	return
	  lhs.snapping_enabled == rhs.snapping_enabled &&
	  lhs.color            == rhs.color &&
	  lhs.display          == rhs.display &&
	  lhs.alignment        == rhs.alignment &&
	  lhs.additional_rotation == rhs.additional_rotation &&
	  lhs.unit             == rhs.unit &&
	  lhs.horz_spacing     == rhs.horz_spacing &&
	  lhs.vert_spacing     == rhs.vert_spacing &&
	  lhs.horz_offset      == rhs.horz_offset &&
	  lhs.vert_offset      == rhs.vert_offset;
}

bool operator!=(const MapGrid& lhs, const MapGrid& rhs)
{
	return !(lhs == rhs);
}
