/*
 *    Copyright 2012 Pete Curtis
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

#include "file_format.h"

#include <cassert>

#include <QFileInfo>

#include "symbol.h"
#include "template.h"

Format::Format(const QString& id, const QString& description, const QString& file_extension, bool supportsImport, bool supportsExport, bool export_lossy)
    : format_id(id), format_description(description), file_extension(file_extension),
      format_filter(QString("%1 (*.%2)").arg(description).arg(file_extension)),
      supports_import(supportsImport), supports_export(supportsExport), export_lossy(export_lossy)
{
}

bool Format::understands(const unsigned char *buffer, size_t sz) const
{
    return false;
}

Importer *Format::createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException)
{
    throw FormatException(QString("Format (%1) does not support import").arg(description()));
}

Exporter *Format::createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FormatException)
{
    throw FormatException(QString("Format (%1) does not support export").arg(description()));
}


void FormatRegistry::registerFormat(Format *format)
{
    fmts.push_back(format);
    if (fmts.size() == 1) default_format_id = format->id();
	assert(findFormatForFilename("filename."+format->fileExtension()) == format);
	assert(findFormatByFilter(format->filter()) == format);
}

const Format *FormatRegistry::findFormat(const QString& id) const
{
    Q_FOREACH(const Format *format, fmts)
    {
        if (format->id() == id) return format;
    }
    return NULL;
}

const Format *FormatRegistry::findFormatByFilter(const QString& filter) const
{
    Q_FOREACH(const Format *format, fmts)
    {
        if (format->filter() == filter) return format;
    }
    return NULL;
}

const Format *FormatRegistry::findFormatForFilename(const QString& filename) const
{
    QString file_extension = QFileInfo(filename).suffix();
    Q_FOREACH(const Format *format, fmts)
    {
        if (format->fileExtension() == file_extension) return format;
    }
    return NULL;
}

FormatRegistry::~FormatRegistry()
{
    for (std::vector<Format *>::reverse_iterator it = fmts.rbegin(); it != fmts.rend(); ++it)
    {
        delete *it;
    }
}

FormatRegistry FileFormats;


void Importer::doImport(bool load_symbols_only, const QString& map_path) throw (FormatException)
{
	import(load_symbols_only);
	
	// Symbol post processing
	for (int i = 0; i < map->getNumSymbols(); ++i)
	{
		if (!map->getSymbol(i)->loadFinished(map))
			throw FormatException(QObject::tr("Error during symbol post-processing."));
	}
	
	// Template loading: try to find all template files
	bool have_lost_template = false;
	for (int i = 0; i < map->getNumTemplates(); ++i)
	{
		Template* temp = map->getTemplate(i);
		
		bool loaded_from_template_dir = false;
		temp->tryToFindAndReloadTemplateFile(map_path, &loaded_from_template_dir);
		if (loaded_from_template_dir)
			addWarning(QObject::tr("Template \"%1\" has been loaded from the map's directory instead of the relative location to the map file where it was previously.").arg(temp->getTemplateFilename()));
		
		if (temp->getTemplateState() != Template::Loaded)
			have_lost_template = true;
	}
	if (have_lost_template)
		addWarning(QObject::tr("At least one template file could not be found. Click the red template name(s) in the Templates -> Template setup window to locate the template file name(s)."));
}
