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


#include "crs_template.h"

#include <vector>

#include <QCompleter>
#include <QLineEdit>
#include <QSpinBox>

#include "../util_gui.h"


std::vector<CRSTemplate*> CRSTemplate::crs_templates;


// ### CRSTemplate::Param ###

CRSTemplate::Param::Param(const QString& desc)
 : desc(desc)
{
	// nothing
}

CRSTemplate::Param::~Param()
{
	// nothing, not inlined
}



// ### CRSTemplate ###

CRSTemplate::CRSTemplate(const QString& id, const QString& name, const QString& coordinates_name, const QString& spec_template)
: id(id)
, name(name)
, coordinates_name(coordinates_name)
, spec_template(spec_template)
{
	// nothing
}

CRSTemplate::~CRSTemplate()
{
	for (std::size_t i = 0; i < params.size(); ++i)
		delete params[i];
}

void CRSTemplate::addParam(Param* param)
{
	params.push_back(param);
}

std::size_t CRSTemplate::getNumCRSTemplates()
{
	return crs_templates.size();
}

CRSTemplate& CRSTemplate::getCRSTemplate(std::size_t index)
{
	return *crs_templates[index];
}

CRSTemplate* CRSTemplate::getCRSTemplate(const QString& id)
{
	for (std::size_t i = 0, end = crs_templates.size(); i < end; ++i)
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
