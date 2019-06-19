/*
 *    Copyright 2013, 2014 Thomas Schöps
 *    Copyright 2014, 2015, 2017-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_CRS_TEMPLATE_IMPLEMENTATION_H
#define OPENORIENTEERING_CRS_TEMPLATE_IMPLEMENTATION_H

#include "crs_template.h"

#include <utility>
#include <vector>

#include <QLineEdit>
#include <QString>
#include <QVariant>

class QWidget;

namespace OpenOrienteering {

class LatLon;


/**
 * This namespace collects CRS template implementations
 */
namespace CRSTemplates {

/**
 * Creates and returns a list of known CRS Templates.
 */
CRSTemplateRegistry::TemplateList defaultList();



/**
 * CRSTemplate parameter specifying a text parameter.
 */
class TextParameter : public CRSTemplateParameter
{
public:
	TextParameter(const QString& id, const QString& name);
	QWidget* createEditor(WidgetObserver& observer) const override;
	QString value(const QWidget* edit_widget) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
	
protected:
	/// The type of editor widget returned from createEditor.
	using Editor = QLineEdit;
	
};



/**
 * CRSTemplate parameter giving a full specification.
 */
class FullSpecParameter : public TextParameter
{
public:
	FullSpecParameter(const QString& id, const QString& name);
	QWidget* createEditor(WidgetObserver& observer) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
};



/**
 * CRSTemplate parameter specifying a UTM zone.
 */
class UTMZoneParameter : public CRSTemplateParameter
{
public:
	UTMZoneParameter(const QString& id, const QString& name);
	QWidget* createEditor(WidgetObserver& observer) const override;
	std::vector<QString> specValues(const QString& edit_value) const override;
	QString value(const QWidget* edit_widget) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
	
	/**
	 * Determines the UTM zone from the given latitude and longitude.
	 * 
	 * Returns a null value on error.
	 */
	static QVariant calculateUTMZone(const LatLon& lat_lon);
};



/**
 * CRSTemplate integer parameter, with values from an integer range.
 */
class IntRangeParameter : public CRSTemplateParameter
{
public:
	using OutputList = std::vector< std::pair<int, int> >;
	
	IntRangeParameter(const QString& id, const QString& name, int min_value, int max_value);
	IntRangeParameter(const QString& id, const QString& name, int min_value, int max_value, OutputList&& outputs);
	QWidget* createEditor(WidgetObserver& observer) const override;
	std::vector<QString> specValues(const QString& edit_value) const override;
	QString value(const QWidget* edit_widget) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
	
private:
	const int min_value;
	const int max_value;
	const OutputList outputs;
};


}  // namespace CRSTemplates

}  // namespace OpenOrienteering

#endif
