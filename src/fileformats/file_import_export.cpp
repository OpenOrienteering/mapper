/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2013, 2014 Thomas Schöps
 *    Copyright 2013-2018 Kai Pastor
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

#include "file_import_export.h"

#include <QFlags>
#include <QLatin1Char>

#include "core/map.h"
#include "core/map_part.h"
#include "core/map_view.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"
#include "core/symbols/point_symbol.h"
#include "core/symbols/symbol.h"
#include "fileformats/file_format.h"
#include "templates/template.h"


namespace OpenOrienteering {

// ### ImportExport ###

ImportExport::~ImportExport() = default;


QVariant ImportExport::option(const QString& name) const
{
	if (!options.contains(name))
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("No such option: %1", "No such import / export option").arg(name));
	return options[name];
}



// ### Importer ###

Importer::~Importer() = default;


void Importer::doImport(bool load_symbols_only, const QString& map_path)
{
	if (view)
		view->setTemplateLoadingBlocked(true);
	
	import(load_symbols_only);
	
	// Object post processing:
	// - make sure that there is no object without symbol
	// - make sure that all area-only path objects are closed
	// - make sure that there are no special points in wrong places (e.g. curve starts inside curves)
	for (int p = 0; p < map->getNumParts(); ++p)
	{
		MapPart* part = map->getPart(p);
		for (int o = 0; o < part->getNumObjects(); ++o)
		{
			Object* object = part->getObject(o);
			if (object->getSymbol() == nullptr)
			{
				addWarning(::OpenOrienteering::Importer::tr("Found an object without symbol."));
				if (object->getType() == Object::Point)
					object->setSymbol(map->getUndefinedPoint(), true);
				else if (object->getType() == Object::Path)
					object->setSymbol(map->getUndefinedLine(), true);
				else
				{
					// There is no undefined symbol for this type of object, delete the object
					part->deleteObject(o, false);
					--o;
					continue;
				}
			}
			
			if (object->getType() == Object::Path)
			{
				PathObject* path = object->asPath();
				auto contained_types = path->getSymbol()->getContainedTypes();
				if (contained_types & Symbol::Area && !(contained_types & Symbol::Line))
					path->closeAllParts();
				
				path->normalize();
			}
		}
	}
	
	if (auto deleted = map->deleteIrregularObjects())
	{
		addWarning(tr("Dropped %n irregular object(s).", nullptr, int(deleted)));
	}
	
	// Symbol post processing
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (!map->getSymbol(i)->loadingFinishedEvent(map))
			throw FileFormatException(::OpenOrienteering::Importer::tr("Error during symbol post-processing."));
	}
	
	// Template loading: try to find all template files
	if (view)
		view->setTemplateLoadingBlocked(false);
	bool have_lost_template = false;
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		Template* temp = map->getTemplate(i);
		bool found_in_map_dir = false;
		if (!temp->tryToFindTemplateFile(map_path, &found_in_map_dir))
		{
			have_lost_template = true;
		}
		else if (!view || view->getTemplateVisibility(temp).visible)
		{
			if (!temp->loadTemplateFile(false))
			{
				addWarning(tr("Failed to load template '%1', reason: %2")
				           .arg(temp->getTemplateFilename(), temp->errorString()));
			}
			else
			{
				auto error_string = temp->errorString();
				if (found_in_map_dir)
				{
					error_string.prepend(
					            ::OpenOrienteering::Importer::tr(
					               "Template \"%1\" has been loaded from the map's directory instead of"
					               " the relative location to the map file where it was previously."
					               ).arg(temp->getTemplateFilename()) + QLatin1Char('\n') );
				}
				
				if (!error_string.isEmpty())
				{
					addWarning(tr("Warnings when loading template '%1':\n%2")
					           .arg(temp->getTemplateFilename(), temp->errorString()));
				}
			}
		}
	}
	
	if (have_lost_template)
	{
#if defined(Q_OS_ANDROID)
		addWarning(tr("At least one template file could not be found."));
#else
		addWarning(tr("At least one template file could not be found.") + QLatin1Char(' ') +
		           tr("Click the red template name(s) in the Templates -> Template setup window to locate the template file name(s)."));
#endif
	}
}

void Importer::finishImport()
{
	// Nothing, not inlined
}



// ### Exporter ###

Exporter::~Exporter() = default;


}  // namespace OpenOrienteering
