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

#include <exception>
#include <memory>

#include <QtGlobal>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFlags>
#include <QIODevice>
#include <QLatin1Char>
#include <QSaveFile>
#include <QScopedValueRollback>

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


bool ImportExport::supportsQIODevice() const noexcept
{
	return true;
}

void ImportExport::setDevice(QIODevice* device)
{
	device_ = device;
}


QVariant ImportExport::option(const QString& name) const
{
	if (!options.contains(name))
		throw FileFormatException(::OpenOrienteering::ImportExport::tr("No such option: %1", "No such import / export option").arg(name));
	return options[name];
}


void ImportExport::setOption(const QString& name, const QVariant& value)
{
	options[name] = value;
}


// ### Importer ###

Importer::~Importer() = default;


void Importer::setLoadSymbolsOnly(bool value)
{
	load_symbols_only = value;
}


bool Importer::doImport()
{
	std::unique_ptr<QFile> managed_file;
	QScopedValueRollback<QIODevice*> original_device{device_};
	if (supportsQIODevice())
	{
		if (!device_)
		{
			managed_file = std::make_unique<QFile>(path);
			device_ = managed_file.get();
		}
		if (!device_->isOpen() && !device_->open(QIODevice::ReadOnly))
		{
			addWarning(tr("Cannot open file\n%1:\n%2").arg(path, device_->errorString()));
			return false;
		}
	}
	
	try
	{
		prepare();
		if (!importImplementation())
		{
			Q_ASSERT(!warnings().empty());
			importFailed();
			return false;
		}
		validate();
	}
	catch (std::exception &e)
	{
		importFailed();
		addWarning(tr("Cannot open file\n%1:\n%2").arg(path, QString::fromLocal8Bit(e.what())));
		return false;
	}
	
	return true;
}


void Importer::prepare()
{
	if (view)
		view->setTemplateLoadingBlocked(true);
}

// Don't add warnings in this function. They may hide the error message.
void Importer::importFailed()
{
	if (view)
		view->setTemplateLoadingBlocked(false);
}

void Importer::validate()
{
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
		if (!temp->tryToFindTemplateFile(path, &found_in_map_dir)
		    && !temp->tryToFindTemplateFile(QFileInfo(path).dir().path(), &found_in_map_dir))
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
	
	if (view)
	{
		view->setPanOffset({0,0});
	}

	// Update all objects without trying to remove their renderables first, this gives a significant speedup when loading large files
	map->updateAllObjects(); // TODO: is the comment above still applicable?
}



// ### Exporter ###

Exporter::~Exporter() = default;


bool Exporter::doExport()
{
	std::unique_ptr<QSaveFile> managed_file;
	QScopedValueRollback<QIODevice*> original_device{device_};
	if (supportsQIODevice())
	{
		if (!device_)
		{
			managed_file = std::make_unique<QSaveFile>(path);
			device_ = managed_file.get();
		}
		if (!device_->isOpen() && !device_->open(QIODevice::WriteOnly))
		{
			addWarning(tr("Cannot save file\n%1:\n%2").arg(path, device_->errorString()));
			return false;
		}
	}
	
	try
	{
		if (!exportImplementation())
		{
			Q_ASSERT(!warnings().empty());
			return false;
		}
		if (managed_file && !managed_file->commit())
		{
			addWarning(tr("Cannot save file\n%1:\n%2").arg(path, managed_file->errorString()));
			return false;
		}
	}
	catch (std::exception &e)
	{
		addWarning(tr("Cannot save file\n%1:\n%2").arg(path, QString::fromLocal8Bit(e.what())));
		return false;
	}
	
	return true;
}


}  // namespace OpenOrienteering
