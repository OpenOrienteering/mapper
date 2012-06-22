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


/** Describes a file format understood by this application. Each file format has an ID (an internal, non-translated
 *  string); a description (translated); a file extension (non-translated); and methods to indicate their support
 *  for import or export. Formats are installed into a FormatRegistry, and can be looked up in a variety of ways.
 *
 *  A typical Format subclass will look like this:
 *
 *  \code
 *  class MyCustomFileFormat {
 *  public:
 *      MyCustomFileFormat : Format("custom", QObject::tr("Custom file"), "custom", true, true) {
 *      }
 *
 *      Importer *createImporter(const QString &path, Map *map, MapView *view) const throw (FormatException) {
 *          return new CustomImporter(path, map, view);
 *      }
 *
 *      Exporter *createExporter(const QString &path, Map *map, MapView *view) const throw (FormatException) {
 *          return new CustomExporter(path, map, view);
 *      }
 *  }
 *  \endcode
 */
class Format
{
public:
    /** Creates a new file format with the given parameters. Don't use a leading dot on the file extension.
     */
    Format(const QString &id, const QString &description, const QString &file_extension, bool supportsImport = true, bool supportsExport = true, bool export_lossy = true);

    virtual ~Format() {}

    /** Returns the internal ID of the file format.
     */
    const QString &id() const { return format_id; }

    /** Returns a short human-readable description of the file format.
     */
    const QString &description() const { return format_description; }

    /** Returns the file extension used by this file format.
     */
    const QString &fileExtension() const { return file_extension; }

    /** Returns the filter that represents this format in file dialogs.
     */
    const QString &filter() const { return format_filter; }

    /** Returns true if this file format supports importing a map from its associated file type.
     */
    bool supportsImport() const { return supports_import; }

    /** Returns true if this file format supports exporting a map to its associated file type.
     */
    bool supportsExport() const { return supports_export; }

    /** Returns true if this file format believes it is capable of understanding a file that
     *  starts with the given byte sequence. "Magic" numbers and version information is commonly
     *  placed at the beginning of a file, and this method is used by the application to pre-screen
     *  for a suitable Importer. If there is any doubt about whether the file format can successfully
     *  process a file, this method should return false.
     */
    virtual bool understands(const unsigned char *buffer, size_t sz) const;

    /** Creates an Importer that will read the file at the given path into the given map and view.
     *  The caller can then call doImport() in the returned object to start the import process. The caller
     *  is responsible for deleting the Importer when it's finished.
     *
     *  If the Importer could not be created, then this method should throw a FormatException.
	 *  TODO: get rid of path parameter (currently required for libocad)
     */
	virtual Importer *createImporter(QIODevice* stream, const QString &path, Map *map, MapView *view) const throw (FormatException);

    /** Returns true if an exporter for this file format is potentially lossy, i.e., if the exported
     *  file cannot fully represent all aspects of the internal OO map objects. This flag is used by
     *  the application to warn the user before saving to a lossy file type.
     */
    inline bool isExportLossy() const { return export_lossy; }

    /** Creates an Exporter that will save the given map and view into a file at the given path.
     *  The caller can then call doExport() in the returned object to start the export process. The caller
     *  is responsible for deleting the Exporter when it's finished.
     *
     *  If the Exporter could not be created, then this method should throw a FormatException.
	 * *  TODO: get rid of path parameter (currently required for libocad)
     */
	virtual Exporter *createExporter(QIODevice* stream, const QString &path, Map *map, MapView *view) const throw (FormatException);

private:
    QString format_id;
    QString format_description;
    QString file_extension;
    QString format_filter;
    bool supports_import;
    bool supports_export;
    bool export_lossy;

};

/** Provides a registry for file formats, and takes ownership of the supported format objects.
 */
class FormatRegistry
{
public:
    /** Creates an empty file format registry.
     */
    FormatRegistry() {};

    /** Destroys a file format registry.
     */
    ~FormatRegistry();

    /** Returns an immutable list of the available file formats.
     */
    inline const std::vector<Format *> &formats() const { return fmts; }

    /** Finds a file format with the given internal ID, or returns NULL if no format
     *  is found.
     */
    const Format *findFormat(const QString &id) const;

    /** Finds a file format which implements the given filter, or returns NULL if no 
	 * format is found.
     */
    const Format *findFormatByFilter(const QString &filter) const;

    /** Finds a file format whose file extension matches the fie extension of the given
     *  path, or returns NULL if no matching format is found.
     */
    const Format *findFormatForFilename(const QString &filename) const;

    /** Returns the ID of default file format for this registry. This will automatically
     *  be set to the first registered format.
     */
    const QString &defaultFormat() const { return default_format_id; }

    /** Registers a new file format. The registry takes ownership of the provided Format.
     */
    void registerFormat(Format *format);

private:
    std::vector<Format *> fmts;
    QString default_format_id;
};

/** A FormatRegistry that is globally defined for convenience. Within the scope of a single
 *  application, you can simply use calls of the form "FileFormats.findFormat(...)".
 */
extern FormatRegistry FileFormats;



/** Abstract base class for both importer and exporters; provides support for configuring the map and
 *  view to manipulate, setting and retrieving options, and collecting a list of warnings.
 *
 *  Subclasses should define default values for options they intend to use in their constructors,
 *  by calling setOption() with the relevant values. There is no such thing as an "implicit default"
 *  for options.
 */
class ImportExport
{
public:
    /** Creates a new importer or exporter with the given file path, map, and view.
     */
	ImportExport(QIODevice* stream, Map *map, MapView *view) : stream(stream), map(map), view(view) {}

    /** Destroys an importer or exporter.
     */
    virtual ~ImportExport() {}

    /** Returns the current list of warnings collected by this object.
     */
    inline const std::vector<QString> &warnings() const { return warn; }

    /** Sets an option in this importer or exporter.
     */
    inline void setOption(const QString &name, QVariant value) { options[name] = value; }

    /** Retrieves the value of an options in this importer or exporter. If the option does not have
     *  a value - either a default value assigned in the constructor, or a custom value assigned
     *  through setOption() - then a FormatException will be thrown.
     */
    QVariant option(const QString &name) const throw (FormatException);

protected:
    /** Adds an import/export warning to the current list of warnings. The provided message
     *  should be translated.
     */
    inline void addWarning(const QString &str) { warn.push_back(str); }

protected:
    /// The input / output stream
    QIODevice* stream;

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
 *  -# The Importer is constructed, with pointers to the map and view. The Importer
 *     should also set default values for any options it will read. The base class
 *     will throw an exception if the importer reads an option that does not have a value.
 *  -# setOption() will be called zero or more times to customize the options.
 *  -# doImport() will be called to perform the initial import. The implementation of
 *     this method will try to populate the map and view from the given file, and may
 *     optionally register action items for the user via addAction().
 *  -# If action items are present, then they will be presented to the user. Each
 *     action item will have its satisfy() method called with the user's choice.
 *  -# finishImport() will be called. If any action items were created, this method
 *     should finish the import based on the values supplied by the user.
 */
class Importer : public ImportExport
{
public:
    /** Creates a new Importer with the given file path, map, and view.
     */
	Importer(QIODevice* stream, Map *map, MapView *view) : ImportExport(stream, map, view) {}

    /** Destroys this Importer.
     */
    virtual ~Importer() {}

    /** Returns the current list of action items.
     */
    inline const std::vector<ImportAction> &actions() const { return act; }

    /** Begins the import process. The implementation of this method should read the file
     *  and populate the map and view from it. If a fatal error is encountered (such as a
     *  missing or corrupt file), than it should throw a FormatException. If the import can
     *  proceed, but information might be lost in the process, than it should call
     *  addWarning() with a translated, useful description of the issue. The line between
     *  errors and warnings is somewhat of a judgement call on the part of the author, but
     *  generally an Importer should not succeed unless the map is populated sufficiently
     *  to be useful.
     */
	virtual void doImport(bool load_symbols_only) throw (FormatException) = 0;

    /** Once all action items are satisfied, this method should be called to complete the
     *  import process. This class defines a default implementation, that does nothing.
     */
    virtual void finishImport() throw (FormatException) {}

protected:
    /** Adds an action item to the current list.
     */
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
    /** Creates a new Importer with the given file path, map, and view.
     */
	Exporter(QIODevice* stream, Map *map, MapView *view) : ImportExport(stream, map, view) {}

    /** Destroys the current Exporter.
     */
    virtual ~Exporter() {}

    /** Exports the map and view to the given file. If a fatal error is encountered (such as a
     *  permission problem), than this method should throw a FormatException. If the export can
     *  proceed, but information might be lost in the process, than it should call
     *  addWarning() with a translated, useful description of the issue.
     */
    virtual void doExport() throw (FormatException) = 0;
};

#endif // IMPORT_EXPORT_H
