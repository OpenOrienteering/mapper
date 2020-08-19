/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2020 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_MAP_H
#define OPENORIENTEERING_TEMPLATE_MAP_H

#include <memory>
#include <vector>

#include <QtGlobal>
#include <QObject>
#include <QString>

#include "templates/template.h"

class QByteArray;
class QPainter;
class QRectF;
class QStringList;
class QWidget;

namespace OpenOrienteering {

class Map;


/**
 * Template displaying a map file.
 */
class TemplateMap : public Template
{
Q_OBJECT
public:
	/**
	 * Returns the filename extensions supported by this template class.
	 */
	static const std::vector<QByteArray>& supportedExtensions();
	
	
	TemplateMap(const QString& path, Map* map);
	
protected:
	TemplateMap(const TemplateMap& proto);
	
public:
	~TemplateMap() override;
	
	TemplateMap* duplicate() const override;
	
	
	const char* getTemplateType() const override;
	
	bool isRasterGraphics() const override;
	
	
	bool loadTemplateFileImpl() override;
	
	bool postLoadSetup(QWidget* dialog_parent, bool& out_center_in_view) override;
	
	void unloadTemplateFileImpl() override;
	
	
	void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, qreal opacity) const override;
	
	QRectF getTemplateExtent() const override;
	
	
	bool hasAlpha() const override;
	
	
	const Map* templateMap() const;
	
	/**
	 * Returns the template's map.
	 * 
	 * The template must be in loaded state before calling this method.
	 * The template will be in unloaded state afterwards.
	 */
	std::unique_ptr<Map> takeTemplateMap();
	
protected:
	Map* templateMap();
	
	void setTemplateMap(std::unique_ptr<Map>&& map);
	
	
	void reloadLater();
	
	void reload();
	
	
	void calculateTransformation();
	
public:
	/**
	 * Returns the template transformation for a pure OCD configuration.
	 * 
	 * OCD templates in OCD files must use the map's scale and georeferencing.
	 * Only the template file's grid parameters are taken into account, i.e.
	 * the template OCD file must be loaded in order to get this transform.
	 * 
	 * If template_map is null (i.e. the template is not in loaded state), this
	 * function returns the current transformation.
	 */
	TemplateTransform transformForOcd() const;
	
	/**
	 * The name of the QObject property which activates a pure OCD transformation.
	 * 
	 * When this property is set for a TemplateMap, it applies transformForOCD()
	 * upon first loading of the file, and resets the property after loading.
	 */
	static const char* ocdTransformProperty();
	
private:
	std::unique_ptr<Map> template_map;
	bool reload_pending = false;
	
	static QStringList locked_maps;
};


}  // namespace OpenOrienteering

#endif
