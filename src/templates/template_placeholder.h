/*
 *    Copyright 2020 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_PLACEHOLDER_H
#define OPENORIENTEERING_TEMPLATE_PLACEHOLDER_H

#include <memory>

#include <QtGlobal>
#include <QByteArray>
#include <QObject>
#include <QString>

#include "templates/template.h"

class QPainter;
class QRectF;
class QXmlStreamReader;
class QXmlStreamWriter;

namespace OpenOrienteering {

class Map;


/**
 * Placeholder for templates which are not loaded regularly/immediately.
 * 
 * This class can be used e.g. by file format implementations to setup template
 * configuration without binding early to a particular template implementation
 * class.
 */
class TemplatePlaceholder : public Template
{
Q_OBJECT
public:
	TemplatePlaceholder(QByteArray original_type, const QString& path, not_null<Map*> map);
	
protected:
	TemplatePlaceholder(const TemplatePlaceholder& proto);
	
public:	
	~TemplatePlaceholder() override;
	
	TemplatePlaceholder* duplicate() const override;
	
	const char* getTemplateType() const override;
	
	
	/**
	 * Create the instance of the actual template implementation class.
	 * 
	 * This function can be called as soon as the template configuration is
	 * complete and the template path is validated, so that the contents of
	 * the template resource can be taken into account for choosing a template
	 * implementation class.
	 * 
	 * In addition to the regular configuration which is passed via XML, this
	 * function also copies all dynamic `property` values from the placeholder
	 * QObject to the actual template implementation object. This can be used
	 * to set additional configuration hints. However, these properties are
	 * copied only after finishing XML configuration. It is possible to pick
	 * them up at data loading time, or by watching out for
	 * QDynamicPropertyChangeEvent.
	 */
	std::unique_ptr<Template> makeActualTemplate() const;
	
	
	/** Always returns true. The placeholder doesn't know the actual property. */
	bool isRasterGraphics() const override;
	
	/** Does nothing. */
	void drawTemplate(QPainter* painter, const QRectF& clip_rect, double scale, bool on_screen, qreal opacity) const override;
	
	
	/** Does nothing. */
	void setTemplateAreaDirty() override;
	
	/** As the actual extent is unknown, returns an very large rectangle. */
	QRectF getTemplateExtent() const override;
	
	
protected:
	void saveTypeSpecificTemplateConfiguration(QXmlStreamWriter& xml) const override;
	
	bool loadTypeSpecificTemplateConfiguration(QXmlStreamReader& xml) override;
	
	
	bool loadTemplateFileImpl() override;
	
	void unloadTemplateFileImpl() override;

private:
	QByteArray original_type;
	QString config;
	
};


}  // namespace OpenOrienteering

#endif
