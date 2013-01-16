/*
 *    Copyright 2012, 2013 Pete Curtis
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

#ifndef _OPENORIENTEERING_FILE_FORMAT_H
#define _OPENORIENTEERING_FILE_FORMAT_H

#include <exception>

#include <QString>

class QIODevice;

class Exporter;
class Importer;
class Map;
class MapView;

/** An exception type thrown by an importer or exporter if it encounters a fatal error.
 */
class FileFormatException : public std::exception
{
public:
	/** Creates a new exception with the given message
	 *  \param message a text describing the exceptional event that has occured.
	 */
	FileFormatException(const QString& message = QString()) throw();
	
	/** Copy-constructor (C++ FAQ 17.17).
	 */
	FileFormatException(const FileFormatException& other) throw();
	
	/** Destroys the exception object.
	 */
	virtual ~FileFormatException() throw();
	
	/** Returns the message as a QString. 
	 */
	inline const QString& message() const throw();
	
	/** Returns the message as a C string.
	 */
	virtual const char* what() const throw();
	
private:
	QString msg;
};


/** Describes a file format understood by this application. Each file format has an ID
 *  (an internal, non-translated string); a description (translated); a file extension
 *  (non-translated); and methods to indicate their support for import or export. Formats
 *  are installed into a FormatRegistry, and can be looked up in a variety of ways.
 *
 *  A typical FileFormat subclass will look like this:
 *
 *  \code
 *  class MyCustomFileFormat : public FileFormat {
 *  public:
 *      MyCustomFileFormat : FileFormat("custom", QObject::tr("Custom file"), "custom", true, true) {
 *      }
 *
 *      Importer *createImporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException) {
 *          return new CustomImporter(stream, map, view);
 *      }
 *
 *      Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException) {
 *          return new CustomExporter(stream, map, view);
 *      }
 *  }
 *  \endcode
 */
class FileFormat
{
public:
	/** File type enumeration. 
	 * 
	 *  Each file type shall use a distinct bit so that they may be OR-combined.
	 * 
	 *  Currently the program is only used for mapping, and "Map" is the only 
	 *  element. "Course" or "Event" are possible additions.
	 */
	enum FileType
	{
		MapFile     = 0x01,
		
		AllFiles    = MapFile,
		
		EndOfFileTypes
	};
	
	/** A type which handles OR-combinations of file types.
	 */
	Q_DECLARE_FLAGS(FileTypes, FileType)
	
	/** File format features.
	 * 
	 *  Each feature shall use a distinct bit so that they may be OR-combined.
	 */
	enum FormatFeatureFlag
	{
		ImportSupported = 0x01,
		ExportSupported = 0x02,
		ExportLossy     = 0x04,
		
		EndOfFormatFeatures
	};
	
	/** A type which handles OR-combinations of format implementation features.
	 */
	Q_DECLARE_FLAGS(FormatFeatures, FormatFeatureFlag)
	
// 	/** Creates a new file format with the given parameters.
// 	 * 
// 	 *  Don't use a leading dot on the file extension.
// 	 */
// 	FileFormat(const QString& id, const QString& description, const QString& file_extension, bool supportsImport = true, bool supportsExport = true, bool export_lossy = true);
// 	
	/** Creates a new file format with the given parameters.
	 * 
	 *  Don't use a leading dot on the file extension.
	 */
	FileFormat(const QString& id, const QString& description, const QString& file_extension, FileTypes file_types, FormatFeatures features);
	
	/** Destroys the file format information. */
	virtual ~FileFormat();
	
	/** Returns the internal ID of the file format.
	 */
	const QString& id() const;
	
	/** Returns a short human-readable description of the file format.
	 */
	const QString& description() const;
	
	/** Returns the file extension used by this file format.
	 */
	const QString& fileExtension() const;
	
	/** Returns the filter that represents this format in file dialogs.
	 */
	const QString& filter() const;
	
	/** Returns true if this file format supports importing a map from its associated file type.
	 */
	bool supportsImport() const;
	
	/** Returns true if this file format supports exporting a map to its associated file type.
	 */
	bool supportsExport() const;
	
	/** Returns true if an exporter for this file format is potentially lossy, i.e., if the exported
	 *  file cannot fully represent all aspects of the internal OO map objects. This flag is used by
	 *  the application to warn the user before saving to a lossy file type.
	 */
	bool isExportLossy() const;
	
	/** Returns true if this file format believes it is capable of understanding a file that
	 *  starts with the given byte sequence. "Magic" numbers and version information is commonly
	 *  placed at the beginning of a file, and this method is used by the application to pre-screen
	 *  for a suitable Importer. If there is any doubt about whether the file format can successfully
	 *  process a file, this method should return false.
	 */
	virtual bool understands(const unsigned char *buffer, size_t sz) const;
	
	/** Creates an Importer that will read a map file from the given stream into the given map and view.
	 *  The caller can then call doImport() in the returned object to start the import process. The caller
	 *  is responsible for deleting the Importer when it's finished.
	 *
	 *  If the Importer could not be created, then this method should throw a FormatException.
	 */
	virtual Importer* createImporter(QIODevice* stream, Map *map, MapView *view) const throw(FileFormatException);
	
	/** Creates an Exporter that will save the given map and view into the given stream.
	 *  The caller can then call doExport() in the returned object to start the export process. The caller
	 *  is responsible for deleting the Exporter when it's finished.
	 *
	 *  If the Exporter could not be created, then this method should throw a FormatException.
	 */
	virtual Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const throw (FileFormatException);
	
private:
	QString format_id;
	QString format_description;
	QString file_extension;
	QString format_filter;
	FileTypes file_types;
	FormatFeatures format_features;
};


// ### FileFormatException inline and header code ###

inline
FileFormatException::FileFormatException(const QString& message) throw()
 : msg(message)
{
	// Nothing
}

inline
FileFormatException::FileFormatException(const FileFormatException& other) throw()
 : msg(other.msg)
{
	// Nothing
}

inline
const QString& FileFormatException::message() const throw()
{
	return msg;
}


// ### FileFormat inline and header code ###

Q_DECLARE_OPERATORS_FOR_FLAGS(FileFormat::FileTypes)

Q_DECLARE_OPERATORS_FOR_FLAGS(FileFormat::FormatFeatures)

inline
const QString& FileFormat::id() const
{
	return format_id;
}

inline
const QString& FileFormat::description() const
{
	return format_description;
}

inline
const QString& FileFormat::fileExtension() const
{
	return file_extension;
}

inline
const QString& FileFormat::filter() const
{
	return format_filter;
}

inline
bool FileFormat::supportsImport() const
{
	return format_features.testFlag(FileFormat::ImportSupported);
}

inline
bool FileFormat::supportsExport() const
{
	return format_features.testFlag(FileFormat::ExportSupported);
}

inline
bool FileFormat::isExportLossy() const
{
	return format_features.testFlag(FileFormat::ExportLossy);
}


#endif // _OPENORIENTEERING_FILE_FORMAT_H
