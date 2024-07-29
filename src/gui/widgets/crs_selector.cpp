/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2015 Kai Pastor
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


#include "crs_selector.h"

#include <iterator>
#include <memory>

#include <QtGlobal>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLatin1Char>
#include <QLayoutItem>
#include <QSignalBlocker>
#include <QVariant>
#include <QWidget>

#include "core/crs_template.h"
#include "core/georeferencing.h"


namespace OpenOrienteering {

// Helper functions for parameter widgets
namespace {

static const char* crsParameterWidgetProperty = "CRS parameter widget";
static const char* crsParameterKeyProperty    = "CRS parameter key";

inline
void setParameterWidget(QWidget* w, bool value = true)
{
	w->setProperty(crsParameterWidgetProperty, QVariant(value));
}

inline
bool isParameterWidget(const QWidget* w)
{
	return w->property(crsParameterWidgetProperty).toBool();
}

inline
void setParameterKey(QWidget* w, const QString& value)
{
	w->setProperty(crsParameterKeyProperty, QVariant(value));
}

}  // namespace



CRSSelector::CRSSelector(const Georeferencing& georef, QWidget* parent)
 : QComboBox(parent)
 , georef(georef)
 , dialog_layout(nullptr)
 , num_custom_items(0)
 , configured_crs(nullptr)
{
	CRSTemplateRegistry const crs_registry;
	for (const auto& crs : crs_registry.list())
	{
		addItem(crs->name(), QVariant(crs->id()));
	}
}

CRSSelector::~CRSSelector() = default;

void CRSSelector::setDialogLayout(QFormLayout* dialog_layout)
{
	Q_ASSERT(dialog_layout && !this->dialog_layout);
	
	this->dialog_layout = dialog_layout;
	
	using TakingIntArgument = void (QComboBox::*)(int);
	connect(this, (TakingIntArgument)&QComboBox::currentIndexChanged, 
	        this, &CRSSelector::crsSelectionChanged);
}


void CRSSelector::addCustomItem(const QString& text, unsigned short id)
{
	insertItem(num_custom_items, text, QVariant(int(id)));
	if (num_custom_items == 0)
		insertSeparator(1);
	++num_custom_items;
}


const CRSTemplate* CRSSelector::currentCRSTemplate() const
{
	const CRSTemplate* crs = nullptr;
	auto item_data = itemData(currentIndex());
	if (item_data.type() == QVariant::String)
		crs = CRSTemplateRegistry().find(item_data.toString());
	return crs;
}

QString CRSSelector::currentCRSSpec() const
{
	QString spec;
	if (auto crs = currentCRSTemplate())
	{
		spec = crs->specificationTemplate();
		auto field_values = parameters();
		Q_ASSERT(field_values.size() == crs->parameters().size());
		
		auto field_value = begin(field_values);
		for (auto&& param : crs->parameters())
		{
			for (auto&& value : param->specValues(*field_value))
				spec = spec.arg(value);
			++field_value;
		}
	}
	return spec;
}

int CRSSelector::currentCustomItem() const
{
	int id = -1;
	QVariant item_data = itemData(currentIndex());
	if (item_data.type() == QVariant::Int)
		id = item_data.toInt();
	return id;
}

void CRSSelector::setCurrentCRS(const CRSTemplate* crs, const std::vector<QString>& values)
{
	Q_ASSERT(crs);
	if (crs)
	{
		Q_ASSERT(crs->parameters().size() == values.size());
		
		int index = findData(QVariant(crs->id()));
		setCurrentIndex(index);
		
		// Explicit call because signals may be blocked.
		if (crs == currentCRSTemplate())
		{
			configureParameterFields(crs, values);
		}
	}
}

void CRSSelector::setCurrentItem(unsigned short id)
{
	QSignalBlocker block(this);
	int index = findData(QVariant(id));
	setCurrentIndex(index);
	
	// Explicit call because signals may be blocked.
	if (currentCustomItem() == int(id))
	{
		configureParameterFields(nullptr, {});
	}
}


std::vector<QString> CRSSelector::parameters() const

{
	std::vector<QString> values;
	auto crs = currentCRSTemplate();
	if (crs && configured_crs == crs)
	{
		values.reserve(crs->parameters().size());
		
		int row;
		QFormLayout::ItemRole role;
		dialog_layout->getWidgetPosition(const_cast<CRSSelector*>(this), &row, &role);
		
		for (auto&& param : crs->parameters())
		{
			++row;
			auto field = dialog_layout->itemAt(row, QFormLayout::FieldRole)->widget();
			Q_ASSERT(isParameterWidget(field));
			values.push_back(param->value(field));
		}
	}
	else if (crs)
	{
		values.resize(crs->parameters().size());
	}
	
	return values;
}

const Georeferencing& CRSSelector::georeferencing() const
{
	return georef;
}

void CRSSelector::crsSelectionChanged()
{
	configureParameterFields();
	emit crsChanged();
}

void CRSSelector::crsParameterEdited()
{
	emit crsChanged();
}


void CRSSelector::configureParameterFields()
{
	if (dialog_layout)
	{
		auto crs = currentCRSTemplate();
		if (!crs)
			configureParameterFields(crs, {});
		else if (crs->id() == georef.getProjectedCRSId())
			configureParameterFields(crs, georef.getProjectedCRSParameters());
		else
			configureParameterFields(crs, std::vector<QString>(crs->parameters().size()));
	}
}

void CRSSelector::configureParameterFields(const CRSTemplate* crs, const std::vector<QString>& values)
{
	if (crs != configured_crs && dialog_layout)
	{
		removeParameterFields();
		addParameterFields(crs);
	}
	
	if (crs && configured_crs == crs && !values.empty())
	{
		int row;
		QFormLayout::ItemRole role;
		dialog_layout->getWidgetPosition(this, &row, &role);
		
		// Set the new parameter values.
		auto parameters = crs->parameters();
		Q_ASSERT(parameters.size() == values.size());
		
		auto parameter = begin(parameters);
		auto value = begin(values);
		for (++row; dialog_layout->rowCount() > row && value != end(values); ++row)
		{
			auto field_item = dialog_layout->itemAt(row, QFormLayout::FieldRole);
			if (!field_item)
				continue;
			
			auto field = field_item->widget();
			if (!field || !isParameterWidget(field))
				continue;
			
			QSignalBlocker block(field);
			(*parameter)->setValue(field, *value);
			++value;
			++parameter;
		}
	}
}

void CRSSelector::addParameterFields(const CRSTemplate* crs)
{
	Q_ASSERT(!configured_crs);
	
	if (dialog_layout && crs && crs != configured_crs)
	{
		int row;
		QFormLayout::ItemRole role;
		dialog_layout->getWidgetPosition(this, &row, &role);
		
		// Add the labels and fields of the new parameters.
		for (auto&& parameter : crs->parameters())
		{
			++row;
			auto field = parameter->createEditor(*this);
			setParameterWidget(field, true);
			setParameterKey(field, parameter->id());
			auto label = new QLabel(parameter->name() + QLatin1Char(':'));
			if (dialog_layout->itemAt(row, QFormLayout::FieldRole))
			{
				dialog_layout->insertRow(row, label, field);
			}
			else
			{
				dialog_layout->setWidget(row, QFormLayout::LabelRole, label);
				dialog_layout->setWidget(row, QFormLayout::FieldRole, field);
			}
		}
		
		configured_crs = crs;
	}
}

void CRSSelector::removeParameterFields()
{
	if (dialog_layout)
	{
		int crs_row;
		QFormLayout::ItemRole role;
		dialog_layout->getWidgetPosition(this, &crs_row, &role);
		
		// Remove the labels and fields of the old parameters.
		for (int row = dialog_layout->rowCount()-1; row > crs_row; --row)
		{
			auto field_item = dialog_layout->itemAt(row, QFormLayout::FieldRole);
			if (!field_item)
				continue;
			
			auto field = field_item->widget();
			if (!field || !isParameterWidget(field))
				continue;
			
			auto label_item = dialog_layout->itemAt(row, QFormLayout::LabelRole);
			if (auto label = label_item->widget())
			{
				delete dialog_layout->takeAt(dialog_layout->indexOf(label));
				delete label;
			}
			
			delete dialog_layout->takeAt(dialog_layout->indexOf(field));
			delete field;
		}
		
		configured_crs = nullptr;
	}
}

void CRSSelector::changeEvent(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::EnabledChange:
		if (dialog_layout)
		{
			bool enabled = isEnabled();
			if (auto label = dialog_layout->labelForField(this))
				label->setEnabled(enabled);
			
			int row;
			QFormLayout::ItemRole role;
			dialog_layout->getWidgetPosition(this, &row, &role);
			
			// Remove the labels and fields of the old parameters.
			for (++row; dialog_layout->rowCount() > row; ++row)
			{
				auto field_item = dialog_layout->itemAt(row, QFormLayout::FieldRole);
				if (!field_item)
					continue;
				
				auto field = field_item->widget();
				if (!field || !isParameterWidget(field))
					continue;
				
				auto label_item = dialog_layout->itemAt(row, QFormLayout::LabelRole);
				if (auto label = label_item->widget())
					label->setEnabled(enabled);
				
				field->setEnabled(enabled);
			}
			
			configured_crs = nullptr;
		}
		break;
	default:
		; // nothing
	}
}


}  // namespace OpenOrienteering
