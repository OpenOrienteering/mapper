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

#include <QFormLayout>
#include <QLabel>

#include "../georeferencing_dialog.h"
#include "../../core/crs_template.h"
#include "../../core/georeferencing.h"
#include "../../util/scoped_signals_blocker.h"


CRSSelector::CRSSelector(const Georeferencing& georef, QWidget* parent)
 : QWidget(parent)
 , georef(georef)
{
	num_custom_items = 0;
	crs_dropdown = new QComboBox();
	for (auto&& temp : CRSTemplateRegistry().list())
	{
		crs_dropdown->addItem(temp->id(), qVariantFromValue<void*>(const_cast<CRSTemplate*>(temp.get())));
	}
	
	connect(crs_dropdown, SIGNAL(currentIndexChanged(int)), this, SLOT(crsDropdownChanged(int)));
	
	layout = NULL;
	crsDropdownChanged(crs_dropdown->currentIndex());
}

void CRSSelector::addCustomItem(const QString& text, int id)
{
	crs_dropdown->insertItem(num_custom_items, text, QVariant(id));
	if (num_custom_items == 0)
		crs_dropdown->insertSeparator(1);
	++num_custom_items;
}

const CRSTemplate* CRSSelector::getSelectedCRSTemplate()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<void*>())
		return NULL;
	return static_cast<CRSTemplate*>(item_data.value<void*>());
}

QString CRSSelector::getSelectedCRSSpec()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<void*>())
		return QString();
	
	CRSTemplate* temp = static_cast<CRSTemplate*>(item_data.value<void*>());
	QString spec = temp->specificationTemplate();
	
	int row = 0;
	for (auto&& param : temp->parameters())
	{
		++row;
		QWidget* edit_widget = layout->itemAt(row, QFormLayout::FieldRole)->widget();
		auto spec_values = param->specValues(param->value(edit_widget));
		for (auto&& value :  spec_values)
			spec = spec.arg(value);
	}
	
	return spec;
}

int CRSSelector::getSelectedCustomItemId()
{
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (!item_data.canConvert<int>())
		return -1;
	return item_data.toInt();
}

void CRSSelector::selectItem(const CRSTemplate* temp)
{
	int index = crs_dropdown->findData(qVariantFromValue<void*>(const_cast<CRSTemplate*>(temp)));
	blockSignals(true);
	crs_dropdown->setCurrentIndex(index);
	blockSignals(false);
}

void CRSSelector::selectCustomItem(int id)
{
	int index = crs_dropdown->findData(QVariant(id));
	blockSignals(true);
	crs_dropdown->setCurrentIndex(index);
	blockSignals(false);
}

int CRSSelector::getNumParams()
{
	auto temp = getSelectedCRSTemplate();
	if (temp == NULL)
		return 0;
	else
		return (int)temp->parameters().size();
}

QString CRSSelector::getParam(int i)
{
	auto temp = getSelectedCRSTemplate();
	Q_ASSERT(temp != NULL);
	Q_ASSERT(i < (int)temp->parameters().size());
	
	int widget_index = 3 + 2 * i;
	return temp->parameters().at(i)->value(layout->itemAt(widget_index)->widget());
}

void CRSSelector::setParam(int i, const QString& value)
{
	auto temp = getSelectedCRSTemplate();
	Q_ASSERT(temp != NULL);
	Q_ASSERT(i < (int)temp->parameters().size());
	
	int widget_index = 3 + 2 * i;
	QWidget* edit_widget = layout->itemAt(widget_index)->widget();
	if (!edit_widget->hasFocus())
	{
		const QSignalBlocker block(edit_widget);
		temp->parameters().at(i)->setValue(edit_widget, value);
	}
}

const Georeferencing& CRSSelector::georeferencing() const
{
	return georef;
}

void CRSSelector::crsDropdownChanged(int index)
{
	Q_UNUSED(index);
	if (layout)
	{
		for (int i = 2 * layout->rowCount() - 1; i >= 2; --i)
			delete layout->takeAt(i)->widget();
		if (layout->count() > 0)
		{
			layout->takeAt(1);
			delete layout->takeAt(0)->widget();
		}
		delete layout;
		layout = NULL;
	}
	
	layout = new QFormLayout();
	layout->addRow(tr("&Coordinate reference system:"), crs_dropdown);
	
	QVariant item_data = crs_dropdown->itemData(crs_dropdown->currentIndex());
	if (item_data.canConvert<void*>())
	{
		CRSTemplate* temp = static_cast<CRSTemplate*>(item_data.value<void*>());
		for (auto&& param : temp->parameters())
		{
			layout->addRow(param->name() + ":", param->createEditor(*this));
		}
	}
	
	setLayout(layout);
	
	emit crsEdited(true);
}

void CRSSelector::crsParameterEdited()
{
	emit crsEdited(false);
}
