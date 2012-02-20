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

#include <QFileInfo>

Format::Format(const QString &id, const QString &description, const QString &file_extension, bool supportsImport, bool supportsExport, bool export_lossy)
    : format_id(id), format_description(description), file_extension(file_extension),
      supports_import(supportsImport), supports_export(supportsExport), export_lossy(export_lossy)
{
}

bool Format::understands(const unsigned char *buffer, size_t sz) const
{
    return false;
}

Importer *Format::createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    throw FormatException(QString("Format (%1) does not support import").arg(description()));
}

Exporter *Format::createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException)
{
    throw FormatException(QString("Format (%1) does not support export").arg(description()));
}


void FormatRegistry::registerFormat(Format *format)
{
    fmts.push_back(format);
    if (fmts.size() == 1) default_format_id = format->id();
}

const Format *FormatRegistry::findFormat(const QString &id) const
{
    Q_FOREACH(const Format *format, fmts)
    {
        if (format->id() == id) return format;
    }
    return NULL;
}

const Format *FormatRegistry::findFormatForFilename(const QString &filename) const
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
