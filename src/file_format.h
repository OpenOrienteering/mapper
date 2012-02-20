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

#ifndef IMPORT_EXPORT_H
#define IMPORT_EXPORT_H

#include <QHash>
#include <QVariant>

#include "map.h"

// Forward declarations
class Importer;
class Exporter;


/** An exception type thrown by an importer or exporter if it encounters a fatal error.
 */
class FormatException : public std::exception
{
public:
    FormatException(const QString &message = QString()) : m(message) {}
    ~FormatException() throw () {}

    inline const QString &message() const { return m; }
    virtual const char *what() const throw() { return m.toLocal8Bit().constData(); }

private:
    QString m;
};


/** Describes a file format understood by this application.
 */
class Format
{
public:
    Format(const QString &id, const QString &description, const QString &file_extension, bool supportsImport = true, bool supportsExport = true);
    virtual ~Format() {}

    const QString &id() const { return format_id; }
    const QString &description() const { return format_description; }
    const QString &fileExtension() const { return file_extension; }

    bool supportsImport() const { return supports_import; }
    bool supportsExport() const { return supports_export; }

    virtual bool understands(const unsigned char *buffer, size_t sz) const;

    virtual Importer *createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException);
    virtual Exporter *createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException);

private:
    QString format_id;
    QString format_description;
    QString file_extension;
    bool supports_import;
    bool supports_export;

};

/** Provides a registry for formats, and takes ownership of the supported format objects.
 */
class FormatRegistry
{
public:
    FormatRegistry() {};
    ~FormatRegistry();

    inline const std::vector<Format *> &formats() const { return fmts; }
    const Format *findFormat(const QString &id) const;
    const Format *findFormatForFilename(const QString &filename) const;
    const QString &defaultFormat() const { return default_format_id; }

    void registerFormat(Format *format);

private:
    std::vector<Format *> fmts;
    QString default_format_id;
};

extern FormatRegistry FileFormats;


/** Base class for both importer and exporters; provides support for configuring the map and
 *  view to manipulate, setting and retrieving options, and collecting a list of warnings.
 *  Also stores the registry of importers and exporters
 */
class ImportExport
{
public:
    ImportExport(const QString &path, Map *map, MapView *view) : path(path), map(map), view(view) {}
    virtual ~ImportExport() {}

    inline const std::vector<QString> &warnings() const { return warn; }
    inline void setOption(const QString &name, QVariant value) { options[name] = value; }
    QVariant option(const QString &name) const throw (FormatException);


protected:
    inline void addWarning(const QString &str) { warn.push_back(str); }

protected:
    /// The file path
    QString path;

    /// The Map to import or export
    Map *map;

    /// The MapView to import or export
    MapView *view;

private:
    /// A list of options for the import/export
    QHash<QString, QVariant> options;

    /// A list of warnings
    std::vector<QString> warn;
};


/** Represents an action that the user must take to successfully complete an import.
 */
class ImportAction
{

};


/** Base class for all importers. An Importer has the following lifecycle:
 *
 *  1. The Importer is constructed, with pointers to the map and view. The Importer
 *     should also set default values for any options it will read. The base class
 *     will throw an exception if the importer reads an option that does not have a value.
 *  2. setOption() will be called zero or more times to customize the options.
 *  3. doImport() will be called to perform the initial import. The implementation of
 *     this method will try to populate the map and view from the given file, and may
 *     optionally register action items for the user via addActionItem().
 *  4. If action items are present, then they will be presented to the user. Each
 *     action item will have its satisfy() method called with the user's choice.
 *  5. finishImport() will be called. If any action items were created, this method
 *     should finish the import based on the values supplied by the user.
 */
class Importer : public ImportExport
{
public:
    Importer(const QString &path, Map *map, MapView *view) : ImportExport(path, map, view) {}
    virtual ~Importer() {}

    inline const std::vector<ImportAction> &actions() const { return act; }

    virtual void doImport() throw (FormatException) = 0;
    virtual void finishImport() throw (FormatException) {}

protected:
    inline void addAction(const ImportAction &action) { act.push_back(action); }

private:
    /// A list of action items that must be resolved before the import can be completed
    std::vector<ImportAction> act;
};


/** Base class for all exporters. An Exporter has the following lifecycle:
 *
 *  1. The Exporter is constructed, with pointers to the filename, map and view. The Exporter
 *     should also set default values for any options it will read. The base class
 *     will throw an exception if the exporter reads an option that does not have a value.
 *  2. setOption() will be called zero or more times to customize the options.
 *  3. doExport() will be called to perform the export.
 */
class Exporter : public ImportExport
{
public:
    Exporter(const QString &path, Map *map, MapView *view) : ImportExport(path, map, view) {}
    virtual ~Exporter() {}

    virtual void doExport() throw (FormatException) = 0;
};




#endif // IMPORT_EXPORT_H
