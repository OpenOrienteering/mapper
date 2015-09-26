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


#ifndef OPENORIENTEERING_CRS_SELECTOR_H
#define OPENORIENTEERING_CRS_SELECTOR_H

#include <QComboBox>

#include "../../core/crs_template.h"

QT_BEGIN_NAMESPACE
class QFormLayout;
QT_END_NAMESPACE

class BaseGeoreferencingDialog;
class CRSTemplate;


/// Combobox for projected coordinate reference system (CRS) selection,
/// with edit widgets below to specify the free parameters, if necessary.
class ProjectedCRSSelector : public QWidget, public CRSParameterWidgetObserver
{
Q_OBJECT
public:
	ProjectedCRSSelector(const Georeferencing& georef, QWidget* parent = NULL);
	
	/// Adds a custom text item at the top which can be identified by the given id.
	void addCustomItem(const QString& text, int id);
	
	
	/// Returns the selected CRS template,
	/// or NULL if a custom item is selected
	const CRSTemplate* getSelectedCRSTemplate();
	
	/// Returns the selected CRS specification string,
	/// or an empty string if a custom item is selected
	QString getSelectedCRSSpec();
	
	/// Returns the id of the selected custom item,
	/// or -1 in case a normal item is selected
	int getSelectedCustomItemId();
	

	/// Selects the given item
	void selectItem(const CRSTemplate* temp);
	
	/// Selects the given item
	void selectCustomItem(int id);
	
	
	/// Returns the number of parameters shown currently
	int getNumParams();
	
	/// Returns the i-th parameters' value (for storage,
	/// not for pasting into the crs specification!)
	QString getParam(int i);
	
	/// Sets the i-th parameters' value.
	/// Does not emit crsEdited().
	void setParam(int i, const QString& value);
	
	
	/**
	 * Returns the current georeferencing.
	 */
	const Georeferencing& georeferencing() const override;
	
signals:
	/// Called when the user edit the CRS.
	/// system_changed is true if the whole system was switched,
	/// if only a parameter was changed it is false.
	void crsEdited(bool system_changed);
	
private slots:
	void crsDropdownChanged(int index);
	void crsParameterEdited() override;
	
private:
	const Georeferencing& georef;
	QComboBox* crs_dropdown;
	int num_custom_items;
	
	QFormLayout* layout;
};

#endif
