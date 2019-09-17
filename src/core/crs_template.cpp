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


#include "crs_template.h"

#include <algorithm>
#include <iterator>

#include <QtGlobal>
#include <QLatin1Char>


namespace OpenOrienteering {

// From crs_template_implementation.h/.cpp
namespace CRSTemplates
{
	CRSTemplateRegistry::TemplateList defaultList();
}



// ### CRSParameterWidgetObserver ###

CRSParameterWidgetObserver::~CRSParameterWidgetObserver() = default;



// ### CRSTemplateParameter ###

CRSTemplateParameter::CRSTemplateParameter(const QString& id, const QString& name)
 : param_id(id)
 , param_name(name)
{
	// nothing
}

CRSTemplateParameter::~CRSTemplateParameter()
{
	// nothing, not inlined
}

std::vector<QString> CRSTemplateParameter::specValues(const QString& edit_value) const
{
	return { edit_value };
}



// ### CRSTemplate ###

CRSTemplate::CRSTemplate(
        const QString& id,
        const QString& name,
        const QString& coordinates_name,
        const QString& spec_template,
        ParameterList&& parameters)
 : template_id(id)
 , template_name(name)
 , coordinates_name(coordinates_name)
 , spec_template(spec_template)
 , params(std::move(parameters))
{
	// nothing
}

CRSTemplate::~CRSTemplate()
{
	for (auto&& param : params)
		delete param;
}

QString CRSTemplate::coordinatesName(const std::vector<QString>& values) const
{
	Q_ASSERT(params.size() == values.size()
	         || values.empty());
	
	auto name = coordinates_name;
	
	auto value = begin(values);
	auto last_value = end(values);
	for (auto key = begin(params), last = end(params);
	     key != last && value != last_value;
	     ++key, ++value)
	{
		name.replace(QLatin1Char('@') + (*key)->id() + QLatin1Char('@'), *value);
	}
	
	return name;
}



// ### CRSTemplateRegistry ###

CRSTemplateRegistry::CRSTemplateRegistry()
{
	static auto shared_list = CRSTemplates::defaultList();
	this->templates = &shared_list;
}

const CRSTemplate* CRSTemplateRegistry::find(const QString& id) const
{
	const CRSTemplate* ret = nullptr;
	for (auto&& temp : *templates)
	{
		if (temp->id() == id) {
			ret = temp.get();
			break;
		}
			
	}
	return ret;
}

void CRSTemplateRegistry::add(std::unique_ptr<const CRSTemplate> temp)
{
	templates->push_back(std::move(temp));
}


}  // namespace OpenOrienteering
