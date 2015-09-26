/*
 *    Copyright 2013, 2013, 2014 Thomas Schöps
 *    Copyright 2014 Kai Pastor
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


#ifndef _OPENORIENTEERING_CRS_TEMPLATE_H_
#define _OPENORIENTEERING_CRS_TEMPLATE_H_

#include <QWidget>


/**
 * A template for a coordinate reference system specification (CRS) string.
 * 
 * A CRSTemplate may contain one or more parameters described by the Param struct.
 * For each param, spec_template must contain a number of free parameters for QString::arg(),
 * e.g. "%1" for the first parameter.
 */
class CRSTemplate
{
public:
	/**
	 * Abstract base class for parameters in CRSTemplates.
	 */
	struct Param
	{
		Param(const QString& desc);
		virtual ~Param();
		/** Must create a widget which can be used to edit the value. */
		virtual QWidget* createEditWidget(QObject* edit_receiver) const = 0;
		/** Must return the widget's value(s) in a form so they can be pasted into
		 *  the CRS specification.
		 */
		virtual std::vector<QString> getSpecValue(QWidget* edit_widget) const = 0;
		/** Must return the widget's value in a form so it can be stored */
		virtual QString getValue(QWidget* edit_widget) const = 0;
		/** Must set the stored value in the widget */
		virtual void setValue(QWidget* edit_widget, const QString& value) = 0;
		
		const QString desc;
	};
	
	/**
	 * Creates a new CRS template.
	 * The id must be unique and different from "Local".
	 */
	CRSTemplate(const QString& id, const QString& name,
				const QString& coordinates_name, const QString& spec_template);
	~CRSTemplate();
	
	/**
	 * Adds a parameter to this template.
	 * A corresponding "%x" (%0, %1, ...) entry must exist in the spec template,
	 * where the parameter value will be pasted using QString.arg() when
	 * applying the CRSTemplate.
	 */
	void addParam(Param* param);
	
	/** Returns the unique ID of this template. */
	inline const QString& getId() const {return id;}
	/** Returns the user-visible name of this template. */
	inline const QString& getName() const {return name;}
	/**
	 * Returns the name for the coordinates of this template, e.g.
	 * "UTM coordinates".
	 */
	inline const QString& getCoordinatesName() const {return coordinates_name;}
	/** Returns the specification string template in Proj.4 format. */
	inline const QString& getSpecTemplate() const {return spec_template;}
	/** Returns the number of free parameters in this template. */
	inline int getNumParams() const {return (int)params.size();}
	/** Returns a reference to the i-th parameter. */
	inline Param& getParam(int index) {return *params[index];}
	
	// CRS Registry
	
	/** Returns the number of CRS templates which are registered */
	static std::size_t getNumCRSTemplates();
	
	/** Returns a registered CRS template by index */
	static CRSTemplate& getCRSTemplate(std::size_t index);
	
	/**
	 * Returns a registered CRS template by id,
	 * or NULL if the given id does not exist
	 */
	static CRSTemplate* getCRSTemplate(const QString& id);
	
	/** Registers a CRS template */
	static void registerCRSTemplate(CRSTemplate* temp);
	
private:
	QString id;
	QString name;
	QString coordinates_name;
	QString spec_template;
	std::vector<Param*> params;
	
	// CRS Registry
	static std::vector<CRSTemplate*> crs_templates;
};

#endif
