/*
 *    Copyright 2015-2019 Kai Pastor
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

#include "crs_param_widgets.h"

#include <QAbstractButton>
#include <QCompleter>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegExp>
#include <QRegExpValidator>
#include <QStringList>
#include <QVariant>

#include "core/crs_template.h"
#include "core/crs_template_implementation.h"
#include "core/georeferencing.h"

// IWYU pragma: no_forward_declare QCompleter
// IWYU pragma: no_forward_declare QHBoxLayout
// IWYU pragma: no_forward_declare QPushButton


namespace OpenOrienteering {

namespace  {

QStringList makeZoneList()
{
	QStringList zone_list;
	zone_list.reserve((60 + 9) * 2);
	for (int i = 1; i <= 60; ++i)
	{
		QString zone = QString::number(i);
		zone_list << QString::fromLatin1("%1 N").arg(zone) << QString::fromLatin1("%1 S").arg(zone);
		if (i < 10)
			zone_list << QString::fromLatin1("0%1 N").arg(zone) << QString::fromLatin1("0%1 S").arg(zone);
	}
	return zone_list;
}

}  // namespace



UTMZoneEdit::UTMZoneEdit(CRSParameterWidgetObserver& observer, QWidget* parent)
 : QWidget(parent)
 , observer(observer)
{
	auto const zone_regexp = QRegExp(QString::fromLatin1("(?:[0-5]?[1-9]|[1-6]0)(?: [NS])?"));
	auto const zone_list = makeZoneList();
	
	line_edit = new QLineEdit();
	line_edit->setValidator(new QRegExpValidator(zone_regexp, line_edit));
	auto* completer = new QCompleter(zone_list, line_edit);
	completer->setMaxVisibleItems(4);
	line_edit->setCompleter(completer);
	connect(line_edit, &QLineEdit::textChanged, this, &UTMZoneEdit::textChanged);
	
	auto* button = new QPushButton(tr("Calculate"));
	connect(button, &QAbstractButton::clicked, this, &UTMZoneEdit::calculateValue);
	
	auto* layout = new QHBoxLayout();
	layout->setMargin(0);
	layout->addWidget(line_edit, 1);
	layout->addWidget(button, 0);
	setLayout(layout);
	
	calculateValue();
}

// This non-inline definition is required to emit a (single) vtable.
UTMZoneEdit::~UTMZoneEdit() = default;


QString UTMZoneEdit::text() const
{
	return line_edit->text();
}

void UTMZoneEdit::setText(const QString& text)
{
	line_edit->setText(text);
}

bool UTMZoneEdit::calculateValue()
{
	auto georef = observer.georeferencing();
	auto zone = CRSTemplates::UTMZoneParameter::calculateUTMZone(georef.getGeographicRefPoint());
	if (!zone.isNull())
	{
		setText(zone.toString());
	}
	
	return !zone.isNull();
}


}  // namespace OpenOrienteering
