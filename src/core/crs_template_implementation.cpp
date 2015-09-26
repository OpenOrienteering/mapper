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


#include "crs_template_implementation.h"

#include <QCompleter>
#include <QLineEdit>
#include <QSpinBox>

#include "georeferencing.h"
#include "../util_gui.h"


namespace CRSTemplates
{

void registerProjectionTemplates()
{
	// TODO: These objects are never deleted.
	
	/*
	 * CRSTemplate(
	 *     id,
	 *     name,
	 *     coordinates name,
	 *     specification string)
	 * 
	 * The id must be unique and different from "Local".
	 */
	
	// UTM
	CRSTemplate* temp = new CRSTemplate(
		"UTM",
		Georeferencing::tr("UTM", "UTM coordinate reference system"),
		Georeferencing::tr("UTM coordinates"),
		"+proj=utm +datum=WGS84 +zone=%1");
	temp->addParam(new CRSTemplates::UTMZoneParam(Georeferencing::tr("UTM Zone (number north/south, e.g. \"32 N\", \"24 S\")")));
	CRSTemplate::registerCRSTemplate(temp);
	
	// Gauss-Krueger
	temp = new CRSTemplate(
		"Gauss-Krueger, datum: Potsdam",
		Georeferencing::tr("Gauss-Krueger, datum: Potsdam", "Gauss-Krueger coordinate reference system"),
		Georeferencing::tr("Gauss-Krueger coordinates"),
		"+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=%2 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs");
	temp->addParam((new CRSTemplates::IntRangeParam(Georeferencing::tr("Zone number (1 to 119)", "Zone number for Gauss-Krueger coordinates"), 1, 119))->clearOutputs()->addDerivedOutput(3, 0)->addDerivedOutput(1000000, 500000));
	CRSTemplate::registerCRSTemplate(temp);
}



// ### UTMZoneParam ###

UTMZoneParam::UTMZoneParam(const QString& desc)
 : CRSTemplate::Param(desc)
{
	// nothing
}

QWidget* UTMZoneParam::createEditWidget(QObject* edit_receiver) const
{
	static const QRegExp zone_regexp("(?:[1-5][0-9]?|60?)(?: [NS])?");
	static QStringList zone_list;
	if (zone_list.isEmpty())
	{
		for (int i = 1; i <= 60; ++i)
		{
			zone_list << QString("%1 N").arg(i) << QString("%1 S").arg(i);
		}
	}
	
	QLineEdit* widget = new QLineEdit();
	widget->setValidator(new QRegExpValidator(zone_regexp, widget));
	QCompleter* completer = new QCompleter(zone_list, widget);
	completer->setMaxVisibleItems(2);
	widget->setCompleter(completer);
	QObject::connect(widget, SIGNAL(textEdited(QString)), edit_receiver, SLOT(crsParamEdited(QString)));
	return widget;
}

std::vector<QString> UTMZoneParam::getSpecValue(QWidget* edit_widget) const
{
	QString zone = getValue(edit_widget);
	zone.replace(" N", "");
	zone.replace(" S", " +south");
	
	std::vector<QString> out;
	out.push_back(zone);
	return out;
}

QString UTMZoneParam::getValue(QWidget* edit_widget) const
{
	QLineEdit* text_edit = static_cast<QLineEdit*>(edit_widget);
	return text_edit->text();
}

void UTMZoneParam::setValue(QWidget* edit_widget, const QString& value)
{
	QLineEdit* text_edit = static_cast<QLineEdit*>(edit_widget);
	text_edit->setText(value);
}



// ### IntRangeParam ###

IntRangeParam::IntRangeParam(const QString& desc, int min_value, int max_value)
: Param(desc)
, min_value(min_value)
, max_value(max_value)
{
	outputs.push_back(std::make_pair(1, 0));
}

QWidget* IntRangeParam::createEditWidget(QObject* edit_receiver) const
{
	QSpinBox* widget = Util::SpinBox::create(min_value, max_value);
	QObject::connect(widget, SIGNAL(valueChanged(QString)), edit_receiver, SLOT(crsParamEdited(QString)));
	return widget;
}

std::vector<QString> IntRangeParam::getSpecValue(QWidget* edit_widget) const
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	int chosen_value = spin_box->value();
	
	std::vector<QString> out;
	for (std::size_t i = 0; i < outputs.size(); ++ i)
	{
		std::pair<int, int> factor_and_bias = outputs[i];
		out.push_back(QString::number(factor_and_bias.first * chosen_value + factor_and_bias.second));
	}
	return out;
}

QString IntRangeParam::getValue(QWidget* edit_widget) const
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	return QString::number(spin_box->value());
}

void IntRangeParam::setValue(QWidget* edit_widget, const QString& value)
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	spin_box->setValue(value.toInt());
}

IntRangeParam* IntRangeParam::clearOutputs()
{
	outputs.clear();
	return this;
}

IntRangeParam* IntRangeParam::addDerivedOutput(int factor, int bias)
{
	outputs.push_back(std::make_pair(factor, bias));
	return this;
}



} // namespace CRSTemplates
