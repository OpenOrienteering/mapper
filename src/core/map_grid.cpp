/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2014-2018 Kai Pastor
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

#include <QtMath>
#include <QPainter>
#include <QXmlStreamReader>

#include "core/georeferencing.h"
#include "core/map.h"
#include "core/map_coord.h"
#include "util/util.h"
#include "util/xml_stream_util.h"


namespace OpenOrienteering {

namespace {

struct ProcessLine
{
	QPainter* painter;
	void processLine(QPointF a, QPointF b);
};

void ProcessLine::processLine(QPointF a, QPointF b)
{
	painter->drawLine(a, b);
}


}  // namespace


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
	XmlElementWriter element{xml, QLatin1String("grid")};
	const auto name_format = qAlpha(color) == 0xff ? QColor::HexRgb : QColor::HexArgb;
	element.writeAttribute(QLatin1String("color"), QColor::fromRgba(color).name(name_format));
	element.writeAttribute(QLatin1String("display"), display);
	element.writeAttribute(QLatin1String("alignment"), alignment);
	element.writeAttribute(QLatin1String("additional_rotation"), additional_rotation);
	element.writeAttribute(QLatin1String("unit"), unit);
	element.writeAttribute(QLatin1String("h_spacing"), horz_spacing);
	element.writeAttribute(QLatin1String("v_spacing"), vert_spacing);
	element.writeAttribute(QLatin1String("h_offset"), horz_offset);
	element.writeAttribute(QLatin1String("v_offset"), vert_offset);
	element.writeAttribute(QLatin1String("snapping_enabled"), snapping_enabled);
}

const MapGrid& MapGrid::load(QXmlStreamReader& xml)
{
	Q_ASSERT(xml.name() == QLatin1String("grid"));
	
	XmlElementReader element(xml);
	QXmlStreamAttributes attributes = xml.attributes();
	color = QColor(element.attribute<QString>(QLatin1String("color"))).rgba();
	display = MapGrid::DisplayMode(element.attribute<int>(QLatin1String("display")));
	alignment = MapGrid::Alignment(element.attribute<int>(QLatin1String("alignment")));
	additional_rotation = element.attribute<double>(QLatin1String("additional_rotation"));
	unit = MapGrid::Unit(element.attribute<int>(QLatin1String("unit")));
	horz_spacing = element.attribute<double>(QLatin1String("h_spacing"));
	vert_spacing = element.attribute<double>(QLatin1String("v_spacing"));
	horz_offset = element.attribute<double>(QLatin1String("h_offset"));
	vert_offset = element.attribute<double>(QLatin1String("v_offset"));
	snapping_enabled = element.attribute<bool>(QLatin1String("snapping_enabled"));
	
	return *this;
}

void MapGrid::draw(QPainter* painter, const QRectF& bounding_box, Map* map, qreal scale_adjustment) const
{
	double final_horz_spacing, final_vert_spacing;
	double final_horz_offset, final_vert_offset;
	double final_rotation;
	calculateFinalParameters(final_horz_spacing, final_vert_spacing, final_horz_offset, final_vert_offset, final_rotation, map);
	
	QPen pen(color);
	if (qIsNull(scale_adjustment))
	{
		// zero-width cosmetic pen (effectively one pixel)
		pen.setWidth(0);
		pen.setCosmetic(true);
	}
	else
	{
		// 0.1 mm wide non-cosmetic pen
		pen.setWidthF(qreal(0.1) / scale_adjustment);
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
		double prev_horz_offset = MapCoordF::dotProduct(MapCoordF(0, -1).rotated(final_rotation), MapCoordF(georeferencing.getMapRefPoint().x(), -1 * georeferencing.getMapRefPoint().y()));
		double target_horz_offset = MapCoordF::dotProduct(MapCoordF(0, -1).rotated(additional_rotation + M_PI / 2), MapCoordF(georeferencing.getProjectedRefPoint().x(), georeferencing.getProjectedRefPoint().y()));
		final_horz_offset -= factor * target_horz_offset - prev_horz_offset;
		
		double prev_vert_offset = MapCoordF::dotProduct(MapCoordF(1, 0).rotated(final_rotation), MapCoordF(georeferencing.getMapRefPoint().x(), -1 * georeferencing.getMapRefPoint().y()));
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


bool MapGrid::hasAlpha() const
{
	return [](auto alpha) {
		return alpha > 0 && alpha < 255;
	}(qAlpha(color));
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


}  // namespace
