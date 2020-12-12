/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2018-2020 Kai Pastor
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

#ifndef OPENORIENTEERING_FILE_FORMAT_H
#define OPENORIENTEERING_FILE_FORMAT_H

#include <exception>
#include <memory>

#include <QtGlobal>
#include <QByteArray>
#include <QFlags>
#include <QList>
#include <QString>
#include <QStringList>

namespace OpenOrienteering {

class Exporter;
class Importer;
class Map;
class MapView;


/** An exception type thrown by an importer or exporter if it encounters a fatal error.
 */
class FileFormatException : public std::exception  // clazy:exclude=copyable-polymorphic
{
public:
	/** Creates a new exception with the given message
	 *  \param message a text describing the exceptional event that has occurred.
	 */
	FileFormatException(const QString& message = QString());
	
	/** Creates a new exception with the given message
	 *  \param message a text describing the exceptional event that has occurred.
	 */
	FileFormatException(const char* message);
	
	/** Copy-constructor (C++ FAQ 17.17).
	 */
	FileFormatException(const FileFormatException& other) noexcept;
	
	/** Destroys the exception object.
	 */
	~FileFormatException() noexcept override;
	
	/** Returns the message as a QString. 
	 */
	const QString& message() const noexcept;
	
	/** Returns the message as a C string.
	 */
	const char* what() const noexcept override;
	
	
	/**
	 * Returns an exception object representing an internal error
	 * in the given function.
	 * 
	 * This is a helper function used by the FILEFORMAT_ASSERT macro.
	 */
	static FileFormatException internalError(const char* function_info);
	
	
private:
	QString const msg;
	QByteArray const msg_c;
};


/**
 * Checks if the condition is true, and raises an exception otherwise.
 * 
 * This macro is to be used by importer and exporter implementations instead
 * of plain assert or Q_ASSERT: In most cases, program consistency is not
 * affected by internal errors of importers or exporters, and so there is no
 * reason to abort the program (debug builds) or to let it run into a crash
 * (release build). However, on hot paths, evaluating the condition must not
 * be expensive.
 */
#define FILEFORMAT_ASSERT(condition) \
	if (Q_UNLIKELY(!(condition))) throw FileFormatException::internalError(Q_FUNC_INFO);



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
 *      MyCustomFileFormat : FileFormat("custom", ::OpenOrienteering::ImportExport::tr("Custom file"), "custom", true, true) {
 *      }
 *
 *      Importer *createImporter(QIODevice* stream, Map *map, MapView *view) const {
 *          return new CustomImporter(stream, map, view);
 *      }
 *
 *      Exporter *createExporter(QIODevice* stream, Map *map, MapView *view) const {
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
		OgrFile     = 0x02,     ///< Geospatial vector data supported by OGR
		
		AllFiles    = MapFile,  ///< All types which can be handled by an editor.
		
		EndOfFileTypes
	};
	
	/** A type which handles OR-combinations of file types.
	 */
	Q_DECLARE_FLAGS(FileTypes, FileType)
	
	
	/** File format features.
	 * 
	 *  Each feature shall use a distinct bit so that they may be OR-combined.
	 */
	enum struct Feature
	{
		FileOpen        = 0x01,  ///< This format supports reading and is to be used for File > Open...
		FileSave        = 0x02,  ///< This format supports writing and is to be used for File > Save
		FileSaveAs      = 0x04,  ///< This format supports writing and is to be used for File > Save as...
		FileImport      = 0x08,  ///< This format supports reading and is to be used for File > Import...
		FileExport      = 0x10,  ///< This format supports writing and is to be used for File > Export...
		
		ReadingLossy   = 0x100,  ///< The importer cannot handle all file format features.
		WritingLossy   = 0x200,  ///< The exporter cannot handle all Mapper features.
		
	};
	
	/** A type which handles OR-combinations of format implementation features.
	 */
	Q_DECLARE_FLAGS(Features, Feature)
	
	
	/**
	 * A type which indicates the level of support for importing a file.
	 * 
	 * If a file format fully supports a file format, errors during import must
	 * be regard as fatal. If the level of support is Unknown, an import can be
	 * attempted, but an import failure allows no conclusion about whether the
	 * file format is actually unsupported or the file contains invalid data for
	 * a supported format.
	 */
	enum ImportSupportAssumption
	{
		NotSupported   = 0,  ///< The FileFormat does not support the file.
		Unknown        = 1,  ///< The FileFormat support cannot be determine in advance.
		FullySupported = 2   ///< The FileFormat supports the file.
	};
	
	
	/** Creates a new file format with the given parameters.
	 * 
	 *  Don't use a leading dot on the file extension.
	 *  
	 */
	FileFormat(FileType file_type, const char* id, const QString& description, const QString& file_extension, Features features);
	
	FileFormat(const FileFormat&) = delete;
	FileFormat(FileFormat&&) = delete;
	
	/** Destroys the file format information. */
	virtual ~FileFormat();
	
	FileFormat& operator=(const FileFormat&) = delete;
	FileFormat& operator=(FileFormat&&) = delete;
	
	/** Registers an alternative file name extension.
	 *  It is used by the filter.
	 */
	void addExtension(const QString& file_extension);
	
	/** Returns the type of file.
	 */
	FileType fileType() const;
	
	/** Returns the internal ID of the file format.
	 */
	const char* id() const;
	
	/** Returns a short human-readable description of the file format.
	 */
	const QString& description() const;
	
	/** Returns the primary file name extension used by this file format.
	 * 
	 *  This shall only be used for constructing new file names. Otherwise
	 *  you will probably need to use fileExtensions().
	 */
	const QString& primaryExtension() const;
	
	/** Returns all file name extension supported by this file format.
	 */
	const QStringList& fileExtensions() const;
	
	/** Returns the filter that represents this format in file dialogs.
	 */
	const QString& filter() const;
	
	
	/** Returns true if this format supports reading a Map from a file.
	 */
	bool supportsReading() const;
	
	/** Returns true if this format supports writing a Map to a file.
	 */
	bool supportsWriting() const;
	
	
	/** Returns true if this format is available for File > Open...
	 */
	bool supportsFileOpen() const { return format_features.testFlag(Feature::FileOpen); }
	
	/** Returns true if this format is available for File > Save
	 */
	bool supportsFileSave() const { return format_features.testFlag(Feature::FileSave); }
	
	/** Returns true if this format is available for File > Save as...
	 */
	bool supportsFileSaveAs() const { return format_features.testFlag(Feature::FileSaveAs); }
	
	/** Returns true if this format is available for File > Import...
	 */
	bool supportsFileImport() const { return format_features.testFlag(Feature::FileImport); }
	
	/** Returns true if this format is available for File > Export...
	 */
	bool supportsFileExport() const { return format_features.testFlag(Feature::FileExport); }
	
	
	/** Returns true if an importer for this file format is potentially lossy.
	 * 
	 *  When the importer is lossy, the created Map may not fully represent all
	 *  features of the file.
	 */
	bool isReadingLossy() const { return format_features.testFlag(Feature::ReadingLossy); }
	
	/** Returns true if an exporter for this file format is potentially lossy.
	 * 
	 *  When the exporter is lossy, the created file may not fully represent all
	 *  features of the Map in the program.
	 */
	bool isWritingLossy() const { return format_features.testFlag(Feature::WritingLossy); }
	
	
	/** 
	 * Determines whether this FileFormat is capable of understanding a file
	 * which starts with the given byte sequence.
	 * 
	 * Magic numbers and version information are commonly placed at the
	 * beginning of a file. This method is used by the application to pre-screen
	 * for a suitable Importer.
	 * 
	 * The default implementation returns Unknown for file formats which support
	 * import, and NotSupported otherwise.
	 */
	virtual ImportSupportAssumption understands(const char* buffer, int size) const;
	
	
	/**
	 * Returns a filepath which ends with one of the formats extensions.
	 * 
	 * If the filepath does not already end with one the extensions,
	 * this function appends the primary extension (separated by dot).
	 */
	QString fixupExtension(QString filepath) const;
	
	
	/**
	 * Creates an Importer that will read a map file from the given stream.
	 * 
	 * The default implementation returns an unset unique_ptr.
	 */
	virtual std::unique_ptr<Importer> makeImporter(const QString& path, Map *map, MapView *view) const;
	
	/** 
	 * Creates an Exporter that will save a map to the given path.
	 * 
	 * The default implementation returns an unset unique_ptr.
	 */
	virtual std::unique_ptr<Exporter> makeExporter(const QString& path, const Map* map, const MapView* view) const;
	
private:
	FileType file_type;
	const char* format_id;
	QString format_description;
	QStringList file_extensions;
	mutable QString format_filter;
	Features format_features;
};


// ### FileFormatException inline and header code ###

inline
FileFormatException::FileFormatException(const QString& message)
 : msg(message)
 , msg_c(message.toLocal8Bit())
{
	// Nothing
}

inline
FileFormatException::FileFormatException(const char* message)
 : msg(QString::fromLatin1(message))
 , msg_c(message)
{
	// Nothing
}

inline
FileFormatException::FileFormatException(const FileFormatException& other) noexcept
 : msg(other.msg)
 , msg_c(other.msg_c)
{
	// Nothing
}

inline
const QString& FileFormatException::message() const noexcept
{
	return msg;
}


// ### FileFormat inline and header code ###

inline
FileFormat::FileType FileFormat::fileType() const
{
	return file_type;
}

inline
const char* FileFormat::id() const
{
	return format_id;
}

inline
const QString& FileFormat::description() const
{
	return format_description;
}

inline
const QString& FileFormat::primaryExtension() const
{
	Q_ASSERT(file_extensions.size() > 0); // by constructor
	return file_extensions[0];
}

inline
const QStringList& FileFormat::fileExtensions() const
{
	return file_extensions;
}


}  // namespace OpenOrienteering


Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::FileFormat::FileTypes)

Q_DECLARE_OPERATORS_FOR_FLAGS(OpenOrienteering::FileFormat::Features)


#endif // OPENORIENTEERING_FILE_FORMAT_H
