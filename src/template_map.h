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


#ifndef _OPENORIENTEERING_TEMPLATE_MAP_H_
#define _OPENORIENTEERING_TEMPLATE_MAP_H_

#include "template.h"

#include <QStringList>

/// A orienteering map used as template
class TemplateMap : public Template
{
Q_OBJECT
public:
	TemplateMap(const QString& filename, Map* map);
	TemplateMap(const TemplateMap& other);
	virtual ~TemplateMap();
	virtual Template* duplicate();
	virtual const QString getTemplateType() {return "TemplateMap";}
	
	virtual bool open(QWidget* dialog_parent, MapView* main_view);
	virtual void drawTemplate(QPainter* painter, QRectF& clip_rect, double scale, float opacity);
	virtual QRectF getExtent();
	virtual bool canBeDrawnOnto() {return false;}
	
protected:
	virtual bool changeTemplateFileImpl(const QString& filename);

private:
	Map* template_map;
	
	static QStringList locked_maps;
};

#endif
