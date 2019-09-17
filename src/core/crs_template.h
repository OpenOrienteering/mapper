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


#ifndef OPENORIENTEERING_CRS_TEMPLATE_H
#define OPENORIENTEERING_CRS_TEMPLATE_H

#include <memory>
#include <vector>

#include <QString>

class QWidget;

namespace OpenOrienteering {

class Georeferencing;


/**
 * Abstract base class for users of CRS parameter widgets.
 */
class CRSParameterWidgetObserver
{
public:
	CRSParameterWidgetObserver() noexcept = default;
	
	CRSParameterWidgetObserver(const CRSParameterWidgetObserver&) = delete;
	CRSParameterWidgetObserver(CRSParameterWidgetObserver&&) = delete;
	
	virtual ~CRSParameterWidgetObserver();
	
	CRSParameterWidgetObserver& operator=(const CRSParameterWidgetObserver&) = delete;
	CRSParameterWidgetObserver& operator=(CRSParameterWidgetObserver&&) = delete;
	
	/**
	 * Informs the observer about a change in a CRS parameter widget.
	 */
	virtual void crsParameterEdited() = 0;
	
	/**
	 * Returns the current georeferencing.
	 * 
	 * CRS parameter widgets may use this georeferencing for calculating values.
	 */
	virtual const Georeferencing& georeferencing() const = 0;
};



/**
 * Abstract base class for parameters in CRSTemplates.
 */
class CRSTemplateParameter
{
public:
	using WidgetObserver = CRSParameterWidgetObserver;
	
	/**
	 * Constructs a new parameter with the given identifier and name.
	 */
	CRSTemplateParameter(const QString& id, const QString& name);
	
	CRSTemplateParameter(const CRSTemplateParameter&) = delete;
	CRSTemplateParameter(CRSTemplateParameter&&) = delete;
	
	/**
	 * Destructor.
	 */
	virtual ~CRSTemplateParameter();
	
	CRSTemplateParameter& operator=(const CRSTemplateParameter&) = delete;
	CRSTemplateParameter& operator=(CRSTemplateParameter&&) = delete;
	
	/**
	 * Returns the parameter's permanent unique ID.
	 */
	QString id() const;
	
	/**
	 * Returns the parameter's display name.
	 */
	QString name() const;
	
	/**
	 * Creates a widget which can be used to edit the value.
	 * 
	 * The widget should be simple in the sense that it can be used as a field
	 * in a QFormLayout, together with the parameter's label.
	 */
	virtual QWidget* createEditor(WidgetObserver& widget_observer) const = 0;
	
	/**
	 * Return a list of actual specification parameters values from a value in
	 * storage format.
	 * 
	 * The default implementation returns a vector which contains just the
	 * single edit_value.
	 */
	virtual std::vector<QString> specValues(const QString& edit_value) const;
	
	/** 
	 * Return the widget's value(s) in form of a single string.
	 * 
	 * This string can be stored and used for restoring the widget.
	 * 
	 * \see CRSTemplateParameter::setValue
	 */
	virtual QString value(const QWidget* edit_widget) const = 0;
	
	/**
	 * Sets the widget to a stored value.
	 * 
	 * \see CRSTemplateParameter::value
	 */
	virtual void setValue(QWidget* edit_widget, const QString& value) = 0;
	
private:
	const QString param_id;
	const QString param_name;
};



/**
 * A template for a coordinate reference system specification (CRS) string.
 * 
 * A CRSTemplate may contain one or more parameters described by the 
 * CRSTemplateParameter class. For each parameter, spec_template must contain a
 * number of free parameters for QString::arg(), e.g. "%1" for the first parameter.
 */
class CRSTemplate
{
public:
	using Parameter = CRSTemplateParameter;
	
	using ParameterList = std::vector<Parameter*>;
	
	/**
	 * Creates a new CRS template.
	 * 
	 * The id must be unique and different from "Local".
	 * The template takes ownership of the parameters in the list.
	 * 
	 * The coordinates_name may contain placeholders written @id@ which refer
	 * to the parameter with the given ID. They can be replaced with actual
	 * parameter values when calling coordinatesName().
	 */
	CRSTemplate(
	        const QString& id,
	        const QString& name,
	        const QString& coordinates_name,
	        const QString& spec_template,
	        ParameterList&& parameters
	);
	
	// Copying of parameters is not implemented here.
	CRSTemplate(const CRSTemplate&) = delete;
	
	/**
	 * Destructor.
	 * 
	 * This deletes the parameters.
	 */
	~CRSTemplate();
	
	// Copying of parameters is not implemented here.
	CRSTemplate& operator=(const CRSTemplate&) = delete;
	
	
	/**
	 * Returns the unique ID of this template.
	 */
	QString id() const;
	
	/**
	 * Returns the display name of this template.
	 */
	QString name() const;
	
	/**
	 * Returns the display name for the coordinates of this template,
	 * e.g. "UTM coordinates".
	 * 
	 * The values list must be either of the same size as the templates list
	 * parameters, or empty. The given parameter values are substituted for the
	 * respective @id@ placeholders in the coordinates_name which was passed to
	 * the constructor.
	 */
	QString coordinatesName(const std::vector<QString>& values = {}) const;
	
	/** 
	 * Returns the specification string template in Proj.4 format.
	 */
	QString specificationTemplate() const;
	
	/**
	 * Returns the parameters.
	 */
	const ParameterList& parameters() const;
	
	
private:
	const QString template_id;
	const QString template_name;
	const QString coordinates_name;
	const QString spec_template;
	const ParameterList params;
};



/**
 * A directory of known CRS templates.
 * 
 * All instances of this class provide access to the same list.
 */
class CRSTemplateRegistry
{
public:
	using TemplateList = std::vector< std::unique_ptr<const CRSTemplate> >;
	
	/**
	 * Creates an object for accessing the CRS template directory.
	 */
	CRSTemplateRegistry();
	
	/**
	 * Returns the list of registered CRS templates.
	 */
	const TemplateList& list() const;
	
	/**
	 * Finds the registered CRS template with the given id,
	 * or returns nullptr if the given id does not exist.
	 */
	const CRSTemplate* find(const QString& id) const;
	
	/**
	 * Registers a CRS template.
	 * 
	 * Note that the directory and thus the template outlives the
	 * CRSTemplateRegistry object.
 	 */
	void add(std::unique_ptr<const CRSTemplate> temp);
	
private:
	TemplateList* templates;
};



//### CRSTemplateParameter inline code ###

inline
QString CRSTemplateParameter::id() const
{
	return param_id;
}

inline
QString CRSTemplateParameter::name() const
{
	return param_name;
}



//### CRSTemplate inline code ###

inline
QString CRSTemplate::id() const
{
	return template_id;
}

inline
QString CRSTemplate::name() const
{
	return template_name;
}

inline
QString CRSTemplate::specificationTemplate() const
{
	return spec_template;
}

inline
const CRSTemplate::ParameterList& CRSTemplate::parameters() const
{
	return params;
}



//### CRSTemplateRegistry inline code ###

inline
const CRSTemplateRegistry::TemplateList& CRSTemplateRegistry::list() const
{
	return *templates;
}


}  // namespace OpenOrienteering

#endif
