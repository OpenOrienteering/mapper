/*
 *    Copyright 2012 Thomas Schöps, Pete Curtis
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

#ifndef NATIVE_FILE_FORMAT_H
#define NATIVE_FILE_FORMAT_H

#include "file_format.h"

class NativeFileFormat : public Format
{
public:
    NativeFileFormat() : Format("native", QObject::tr("Open Orienteering Mapper"), "omap", true, true) {}

    bool understands(const unsigned char *buffer, size_t sz) const;
    Importer *createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException);
    Exporter *createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException);

    static const int least_supported_file_format_version;
    static const int current_file_format_version;
    static const char magic_bytes[4];
};


class NativeFileImport : public Importer
{
public:
    NativeFileImport(const QString &path, Map *map, MapView *view);
    ~NativeFileImport();

    void doImport() throw (FormatException);
};


class NativeFileExport : public Exporter
{
public:
    NativeFileExport(const QString &path, Map *map, MapView *view);
    ~NativeFileExport();

    void doExport() throw (FormatException);
};


#endif // NATIVE_FILE_FORMAT_H
