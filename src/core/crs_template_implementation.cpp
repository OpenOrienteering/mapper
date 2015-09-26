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
#include <QVariant>

#include "georeferencing.h"
#include "../gui/widgets/crs_param_widgets.h"
#include "../gui/georeferencing_dialog.h"
#include "../util_gui.h"
#include "../util/scoped_signals_blocker.h"


namespace CRSTemplates
{

/// \todo Replace local definition by C++14 std::make_unique.
///       For gcc, this requires at least version 4.9.
template <class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}



CRSTemplateRegistry::TemplateList defaultList()
{
	CRSTemplateRegistry::TemplateList templates;
	templates.reserve(5);
	
	// UTM
	auto temp = make_unique<CRSTemplate>(
		"UTM",
		Georeferencing::tr("UTM", "UTM coordinate reference system"),
		Georeferencing::tr("UTM coordinates"),
		"+proj=utm +datum=WGS84 +zone=%1",
	    CRSTemplate::ParameterList {
	        new UTMZoneParameter("zone", Georeferencing::tr("UTM Zone (number north/south)"))
	    } );
	templates.push_back(std::move(temp));
	
	// Gauss-Krueger
	temp = make_unique<CRSTemplate>(
		"Gauss-Krueger, datum: Potsdam",
		Georeferencing::tr("Gauss-Krueger, datum: Potsdam", "Gauss-Krueger coordinate reference system"),
		Georeferencing::tr("Gauss-Krueger coordinates"),
		"+proj=tmerc +lat_0=0 +lon_0=%1 +k=1.000000 +x_0=%2 +y_0=0 +ellps=bessel +datum=potsdam +units=m +no_defs",
	    CRSTemplate::ParameterList {
	        new IntRangeParameter("zone", Georeferencing::tr("Zone number (1 to 119)", "Zone number for Gauss-Krueger coordinates"),
	                              1, 119, { {3, 0}, {1000000, 500000} })
	    } );
	templates.push_back(std::move(temp));
	
	// EPSG
	temp = make_unique<CRSTemplate>(
		"EPSG",
		Georeferencing::tr("by EPSG code", "as in: The CRS is specified by EPSG code"),
		Georeferencing::tr("Local coordinates"),
		"+init=epsg:%1",
	    CRSTemplate::ParameterList {
	        new IntRangeParameter("code", Georeferencing::tr("EPSG code"), 1000, 99999)
	    } );
	templates.push_back(std::move(temp));
	
	// Custom
	temp = make_unique<CRSTemplate>(
		"PROJ.4", // Don't change this ID.
		Georeferencing::tr("Custom PROJ.4", "PROJ.4 specification"),
		Georeferencing::tr("Local coordinates"),
		"%1",
	    CRSTemplate::ParameterList {
	        new FullSpecParameter("spec", Georeferencing::tr("Specification", "PROJ.4 specification"))
	    } );
	templates.push_back(std::move(temp));
	
	return templates;
}



// ### TextParameter ###

TextParameter::TextParameter(const QString& key, const QString& name)
 : CRSTemplateParameter(key, name)
{
	// nothing
}

QWidget* TextParameter::createEditor(WidgetObserver& observer) const
{
	auto widget = new TextParameter::Editor();
	QObject::connect(widget, &TextParameter::Editor::textChanged, [&observer](){ observer.crsParameterEdited(); });
	return widget;
}

QString TextParameter::value(const QWidget* edit_widget) const
{
	QString value;
	auto field = qobject_cast<const QLineEdit*>(edit_widget);
	if (field)
		value = field->text();
	return value;
}

void TextParameter::setValue(QWidget* edit_widget, const QString& value)
{
	auto field = qobject_cast<QLineEdit*>(edit_widget);
	if (field)
		field->setText(value);
}



// ### FullSpecParameter ###

FullSpecParameter::FullSpecParameter(const QString& key, const QString& name)
 : TextParameter(key, name)
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

namespace
{

QStringList utmZoneList()
{
	QStringList zone_list;
	for (int i = 1; i <= 60; ++i)
	{
		zone_list << QString("%1 N").arg(i) << QString("%1 S").arg(i);
	}
	return zone_list;
}

} // namespace

UTMZoneParameter::UTMZoneParameter(const QString& key, const QString& name)
 : CRSTemplateParameter(key, name)
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
	zone.replace(" N", "");
	zone.replace(" S", " +south");
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
	if (abs(lat) < 84.0)
	{
		const double lon = lat_lon.longitude();
		int zone_no = int(floor(lon) + 180) / 6 % 60 + 1;
		if (zone_no == 31 && lon >= 3.0 && lat >= 56.0 && lat < 64.0)
			zone_no = 32; // South Norway
		else if (lat >= 72.0 && lon >= 3.0 && lon <= 39.0)
			zone_no = 2 * (int(floor(lon) + 3.0) / 12) + 31; // Svalbard
		QString zone = QString::number(zone_no);
		if (zone_no < 10)
			zone.prepend('0');
		zone.append((lat >= 0.0) ? " N" : " S");
		ret = zone;
	}
	
	return ret;
}



// ### IntRangeParameter ###

IntRangeParameter::IntRangeParameter(const QString& key, const QString& name, int min_value, int max_value)
 : IntRangeParameter(key, name, min_value, max_value, { {1, 0} })
{
	// nothing
}

IntRangeParameter::IntRangeParameter(const QString& key, const QString& name, int min_value, int max_value, OutputList&& outputs)
 : CRSTemplateParameter(key, name)
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



} // namespace CRSTemplates
