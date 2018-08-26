/*
 *    Copyright 2013, 2014 Thomas Schöps
 *    Copyright 2014-2017 Kai Pastor
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

#include <cmath>
#include <memory>

#include <QtGlobal>
#include <QLatin1String>
#include <QLineEdit>
#include <QObject>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVariant>
#include <QWidget>

#include "core/crs_template.h"
#include "core/georeferencing.h"
#include "core/latlon.h"
#include "gui/util_gui.h"
#include "gui/widgets/crs_param_widgets.h"


namespace OpenOrienteering {

namespace CRSTemplates {

CRSTemplateRegistry::TemplateList defaultList()
{
	CRSTemplateRegistry::TemplateList templates;
	templates.reserve(5);
	
	// UTM
	auto temp = std::make_unique<CRSTemplate>(
	  QString::fromLatin1("UTM"),
	  ::OpenOrienteering::Georeferencing::tr("UTM", "UTM coordinate reference system"),
	  ::OpenOrienteering::Georeferencing::tr("UTM coordinates"),
	  QString::fromLatin1("+proj=utm +datum=WGS84 +zone=%1"),
	  CRSTemplate::ParameterList {
	    new UTMZoneParameter(QString::fromLatin1("zone"), ::OpenOrienteering::Georeferencing::tr("UTM Zone (number north/south)"))
	  } );
	templates.push_back(std::move(temp));
	
	// Gauss-Krueger
	temp = std::make_unique<CRSTemplate>(
	  QString::fromLatin1("Gauss-Krueger, datum: Potsdam"),
	  ::OpenOrienteering::Georeferencing::tr("Gauss-Krueger, datum: Potsdam", "Gauss-Krueger coordinate reference system"),
	  ::OpenOrienteering::Georeferencing::tr("Gauss-Krueger coordinates"),
	  QString::fromLatin1("+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=%2 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs"),
	  CRSTemplate::ParameterList {
	    new IntRangeParameter(QString::fromLatin1("zone"), ::OpenOrienteering::Georeferencing::tr("Zone number (1 to 119)", "Zone number for Gauss-Krueger coordinates"),
	                          1, 119, { {3, 0}, {1000000, 500000} })
	  } );
	templates.push_back(std::move(temp));
	
	// EPSG
	temp = std::make_unique<CRSTemplate>(
	  QString::fromLatin1("EPSG"),
	  ::OpenOrienteering::Georeferencing::tr("by EPSG code", "as in: The CRS is specified by EPSG code"),
	  //: Don't translate @code@. It is placeholder.
	  ::OpenOrienteering::Georeferencing::tr("EPSG @code@ coordinates"),
	  QString::fromLatin1("+init=epsg:%1"),
	  CRSTemplate::ParameterList {
	    new IntRangeParameter(QString::fromLatin1("code"), ::OpenOrienteering::Georeferencing::tr("EPSG code"), 1000, 99999)
	  } );
	templates.push_back(std::move(temp));
	
	// Custom
	temp = std::make_unique<CRSTemplate>(
	  QString::fromLatin1("PROJ.4"), // Don't change this ID.
	  ::OpenOrienteering::Georeferencing::tr("Custom PROJ.4", "PROJ.4 specification"),
	  ::OpenOrienteering::Georeferencing::tr("Local coordinates"),
	  QString::fromLatin1("%1"),
	  CRSTemplate::ParameterList {
	    new FullSpecParameter(QString::fromLatin1("spec"), ::OpenOrienteering::Georeferencing::tr("Specification", "PROJ.4 specification"))
	  } );
	templates.push_back(std::move(temp));
	
	return templates;
}



// ### TextParameter ###

TextParameter::TextParameter(const QString& id, const QString& name)
 : CRSTemplateParameter(id, name)
{
	// nothing
}

QWidget* TextParameter::createEditor(WidgetObserver& observer) const
{
	auto widget = new Editor();
	QObject::connect(widget, &TextParameter::Editor::textChanged, [&observer](){ observer.crsParameterEdited(); });
	return widget;
}

QString TextParameter::value(const QWidget* edit_widget) const
{
	QString value;
	auto field = qobject_cast<const Editor*>(edit_widget);
	if (field)
		value = field->text();
	return value;
}

void TextParameter::setValue(QWidget* edit_widget, const QString& value)
{
	auto field = qobject_cast<Editor*>(edit_widget);
	if (field)
		field->setText(value);
}



// ### FullSpecParameter ###

FullSpecParameter::FullSpecParameter(const QString& id, const QString& name)
 : TextParameter(id, name)
{
	// nothing
}

QWidget* FullSpecParameter::createEditor(WidgetObserver& observer) const
{
	auto widget = qobject_cast<TextParameter::Editor*>(TextParameter::createEditor(observer));
	Q_ASSERT(widget);
	if (widget)
	{
		const QSignalBlocker block(widget);
		widget->setText(observer.georeferencing().getProjectedCRSSpec());
	}
	return widget;
}

void FullSpecParameter::setValue(QWidget* edit_widget, const QString& value)
{
	// Don't accidently clear this field.
	if (!value.isEmpty())
		TextParameter::setValue(edit_widget, value);
}



// ### UTMZoneParameter ###

UTMZoneParameter::UTMZoneParameter(const QString& id, const QString& name)
 : CRSTemplateParameter(id, name)
{
	// nothing
}

UTMZoneParameter::~UTMZoneParameter()
{
	// nothing
}

QWidget* UTMZoneParameter::createEditor(WidgetObserver& observer) const
{
	auto widget = new UTMZoneEdit(observer, nullptr);
	QObject::connect(widget, &UTMZoneEdit::textEdited, [&observer](){ observer.crsParameterEdited(); });
	return widget;
}

std::vector<QString> UTMZoneParameter::specValues(const QString& edit_value) const
{
	auto zone = QString { edit_value };
	zone.remove(QLatin1String(" N"));
	zone.replace(QLatin1String(" S"), QLatin1String(" +south"));
	return { zone };
}

QString UTMZoneParameter::value(const QWidget* edit_widget) const
{
	QString value;
	if (auto text_edit = qobject_cast<const UTMZoneEdit*>(edit_widget))
		value = text_edit->text();
	return value;
}

void UTMZoneParameter::setValue(QWidget* edit_widget, const QString& value)
{
	// Don't accidently clear this field.
	auto text_edit = qobject_cast<UTMZoneEdit*>(edit_widget);
	if (text_edit && !value.isEmpty())
		text_edit->setText(value);
}

QVariant UTMZoneParameter::calculateUTMZone(const LatLon lat_lon)
{
	QVariant ret;
	
	const double lat = lat_lon.latitude();
	if (fabs(lat) < 84.0)
	{
		const double lon = lat_lon.longitude();
		int zone_no = int(floor(lon) + 180) / 6 % 60 + 1;
		if (zone_no == 31 && lon >= 3.0 && lat >= 56.0 && lat < 64.0)
			zone_no = 32; // South Norway
		else if (lat >= 72.0 && lon >= 3.0 && lon <= 39.0)
			zone_no = 2 * (int(floor(lon) + 3.0) / 12) + 31; // Svalbard
		QString zone = QString::number(zone_no);
		if (zone_no < 10)
			zone.prepend(QLatin1String("0"));
		zone.append((lat >= 0.0) ? QLatin1String(" N") : QLatin1String(" S"));
		ret = zone;
	}
	
	return ret;
}



// ### IntRangeParameter ###

IntRangeParameter::IntRangeParameter(const QString& id, const QString& name, int min_value, int max_value)
 : IntRangeParameter(id, name, min_value, max_value, { {1, 0} })
{
	// nothing
}

IntRangeParameter::IntRangeParameter(const QString& id, const QString& name, int min_value, int max_value, OutputList&& outputs)
 : CRSTemplateParameter(id, name)
 , min_value(min_value)
 , max_value(max_value)
 , outputs(std::move(outputs))
{
	// nothing
}

IntRangeParameter::~IntRangeParameter()
{
	// nothing
}

QWidget* IntRangeParameter::createEditor(WidgetObserver& observer) const
{
	using TakingIntArgument = void (QSpinBox::*)(int);
	auto widget = Util::SpinBox::create(min_value, max_value);
	QObject::connect(widget, (TakingIntArgument) &QSpinBox::valueChanged, [&observer](){ observer.crsParameterEdited(); });
	return widget;
}

std::vector<QString> IntRangeParameter::specValues(const QString& edit_value) const
{
	auto chosen_value = edit_value.toInt();
	
	std::vector<QString> spec_values;
	spec_values.reserve(outputs.size());
	for (auto&& factor_and_bias : outputs)
	{
		spec_values.push_back(QString::number(factor_and_bias.first * chosen_value + factor_and_bias.second));
	}
	return spec_values;
}

QString IntRangeParameter::value(const QWidget* edit_widget) const
{
	QString value;
	if (auto spin_box = qobject_cast<const QSpinBox*>(edit_widget))
		value = spin_box->cleanText();
	return value;
}

void IntRangeParameter::setValue(QWidget* edit_widget, const QString& value)
{
	if (auto spin_box = qobject_cast<QSpinBox*>(edit_widget))
		spin_box->setValue(value.toInt());
}


}  // namespace CRSTemplates

}  // namespace OpenOrienteering
