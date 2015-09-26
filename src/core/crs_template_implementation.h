/*
 *    Copyright 2013, 2014 Thomas Schöps
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


#ifndef _OPENORIENTEERING_CRS_TEMPLATE_IMPLEMENTATION_H_
#define _OPENORIENTEERING_CRS_TEMPLATE_IMPLEMENTATION_H_


#include "crs_template.h"


/**
 * This namespace collects CRS templates implementation.
 */
namespace CRSTemplates
{

/**
 * Registers a list of known CRS Templates.
 */
void registerProjectionTemplates();



/**
 * CRSTemplate parameter specifying an UTM zone.
 */
class UTMZoneParam : public CRSTemplate::Param
{
public:
	UTMZoneParam(const QString& desc);
	QWidget* createEditWidget(QObject* edit_receiver) const override;
	std::vector<QString> getSpecValue(QWidget* edit_widget) const override;
	QString getValue(QWidget* edit_widget) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
};



/**
 * CRSTemplate integer parameter, with values from an integer range.
 */
class IntRangeParam : public CRSTemplate::Param
{
public:
	IntRangeParam(const QString& desc, int min_value, int max_value);
	QWidget* createEditWidget(QObject* edit_receiver) const override;
	std::vector<QString> getSpecValue(QWidget* edit_widget) const override;
	QString getValue(QWidget* edit_widget) const override;
	void setValue(QWidget* edit_widget, const QString& value) override;
	
	// These methods return this to allow for stacking.
	IntRangeParam* clearOutputs();
	IntRangeParam* addDerivedOutput(int factor, int bias);
	
private:
	const int min_value;
	const int max_value;
	std::vector<std::pair<int, int> > outputs;
};

} // namespace CRSTemplates

#endif
