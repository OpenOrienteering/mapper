/*
 *    Copyright 2013, 2013, 2014 Thomas Schöps
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


#include "crs_template.h"

#include <vector>

#include <QLineEdit>
#include <QSpinBox>

#include "../util_gui.h"


std::vector<CRSTemplate*> CRSTemplate::crs_templates;

CRSTemplate::Param::Param(const QString& desc)
 : desc(desc)
{
}

CRSTemplate::ZoneParam::ZoneParam(const QString& desc)
 : Param(desc)
{
}
QWidget* CRSTemplate::ZoneParam::createEditWidget(QObject* edit_receiver) const
{
	QLineEdit* widget = new QLineEdit();
	QObject::connect(widget, SIGNAL(textEdited(QString)), edit_receiver, SLOT(crsParamEdited(QString)));
	return widget;
}
QString CRSTemplate::ZoneParam::getSpecValue(QWidget* edit_widget) const
{
	QString zone = getValue(edit_widget);
	zone.replace(" N", "");
	zone.replace(" S", " +south");
	return zone;
}
QString CRSTemplate::ZoneParam::getValue(QWidget* edit_widget) const
{
	QLineEdit* text_edit = static_cast<QLineEdit*>(edit_widget);
	return text_edit->text();
}
void CRSTemplate::ZoneParam::setValue(QWidget* edit_widget, const QString& value)
{
	QLineEdit* text_edit = static_cast<QLineEdit*>(edit_widget);
	text_edit->setText(value);
}

CRSTemplate::IntRangeParam::IntRangeParam(const QString& desc, int min_value, int max_value, int apply_factor)
: Param(desc), min_value(min_value), max_value(max_value), apply_factor(apply_factor)
{
}
QWidget* CRSTemplate::IntRangeParam::createEditWidget(QObject* edit_receiver) const
{
	QSpinBox* widget = Util::SpinBox::create(min_value, max_value);
	QObject::connect(widget, SIGNAL(valueChanged(QString)), edit_receiver, SLOT(crsParamEdited(QString)));
	return widget;
}
QString CRSTemplate::IntRangeParam::getSpecValue(QWidget* edit_widget) const
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	return QString::number(apply_factor * spin_box->value());
}
QString CRSTemplate::IntRangeParam::getValue(QWidget* edit_widget) const
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	return QString::number(spin_box->value());
}
void CRSTemplate::IntRangeParam::setValue(QWidget* edit_widget, const QString& value)
{
	QSpinBox* spin_box = static_cast<QSpinBox*>(edit_widget);
	spin_box->setValue(value.toInt());
}

CRSTemplate::CRSTemplate(const QString& id, const QString& name, const QString& coordinates_name, const QString& spec_template)
: id(id), name(name), coordinates_name(coordinates_name), spec_template(spec_template)
{
}

CRSTemplate::~CRSTemplate()
{
	for (int i = 0; i < (int)params.size(); ++i)
		delete params[i];
}

void CRSTemplate::addParam(Param* param)
{
	params.push_back(param);
}

int CRSTemplate::getNumCRSTemplates()
{
	return (int)crs_templates.size();
}

CRSTemplate& CRSTemplate::getCRSTemplate(int index)
{
	return *crs_templates[index];
}

CRSTemplate* CRSTemplate::getCRSTemplate(const QString& id)
{
	for (size_t i = 0, end = crs_templates.size(); i < end; ++i)
	{
		if (crs_templates[i]->getId() == id)
			return crs_templates[i];
	}
	return NULL;
}

void CRSTemplate::registerCRSTemplate(CRSTemplate* temp)
{
	crs_templates.push_back(temp);
}
