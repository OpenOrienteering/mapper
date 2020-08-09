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
