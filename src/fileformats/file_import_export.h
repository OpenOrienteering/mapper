/*
 *    Copyright 2012, 2013 Pete Curtis
 *    Copyright 2018 Kai Pastor
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

#ifndef OPENORIENTEERING_IMPORT_EXPORT_H
#define OPENORIENTEERING_IMPORT_EXPORT_H

#include <vector>

#include <QCoreApplication>
#include <QHash>
#include <QString>
#include <QVariant>

class QIODevice;

namespace OpenOrienteering {

class Map;
class MapView;


/**
 * Abstract base class for both importer and exporters.
 * 
 * This class provides support for setting and retrieving options, for
 * collecting a list of warnings, and for storing a final error message.
 * 
 * Subclass constructors need to define default values for options they use,
 * by calling setOption().
 */
class ImportExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::ImportExport)
	
public:
	/** Destroys an importer or exporter.
	 */
	virtual ~ImportExport();
	
	// Other default special member functions are fine.
	
	
	/** Returns the current list of warnings collected by this object.
	 */
	const std::vector<QString> &warnings() const;
	
	/** Sets an option in this importer or exporter.
	 */
	void setOption(const QString& name, const QVariant& value);
	
	/** Retrieves the value of an options in this importer or exporter. If the option does not have
	 *  a value - either a default value assigned in the constructor, or a custom value assigned
	 *  through setOption() - then a FormatException will be thrown.
	 */
	QVariant option(const QString& name) const;
	
	protected:
	/** Adds an import/export warning to the current list of warnings. The provided message
	 *  should be translated.
	 */
	void addWarning(const QString& str);
	
private:
	/// A list of options for the import/export
	QHash<QString, QVariant> options;
	
	/// A list of warnings
	std::vector<QString> warn;
};


/**
 * Represents an action that the user must take to successfully complete an import.
 * TODO: Currently not fully implemented, as this should be done using the
 * planned ProblemWidget instead, which works independently of import / export.
 */
class ImportAction
{
	// Nothing
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
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::Importer)
	
public:
	/** Creates a new Importer with the given input stream, map, and view.
	 */
	Importer(QIODevice* stream, Map *map, MapView *view)
	: stream(stream), map(map), view(view)
	{}
	
	/** Destroys this Importer.
	 */
	~Importer() override;
	
	/** Returns the current list of action items.
	 */
	inline const std::vector<ImportAction> &actions() const;
	
	/** Begins the import process. The implementation of this method should read the file
	 *  and populate the map and view from it. If a fatal error is encountered (such as a
	 *  missing or corrupt file), than it should throw a FormatException. If the import can
	 *  proceed, but information might be lost in the process, than it should call
	 *  addWarning() with a translated, useful description of the issue. The line between
	 *  errors and warnings is somewhat of a judgement call on the part of the author, but
	 *  generally an Importer should not succeed unless the map is populated sufficiently
	 *  to be useful.
	 */
	void doImport(bool load_symbols_only, const QString& map_path = QString());
	
	/** Once all action items are satisfied, this method should be called to complete the
	 *  import process. This class defines a default implementation, that does nothing.
	 */
	virtual void finishImport();
	
protected:
	/** Implementation of doImport().
	 */
	virtual void import(bool load_symbols_only) = 0;
	
	/** Adds an action item to the current list.
	 */
	inline void addAction(const ImportAction &action);
	
	
	/// The input / output stream
	QIODevice* stream;
	
	/// The Map to import or export
	Map* map;
	
	/// The MapView to import or export
	MapView* view;
	
	
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
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::Exporter)
	
public:
	/** Creates a new Exporter with the given output stream, map, and view.
	 */
	Exporter(QIODevice* stream, const Map* map, const MapView* view)
	: stream(stream), map(map), view(view)
	{}
	
	/** Destroys the current Exporter.
	 */
	~Exporter() override;
	
	/** Exports the map and view to the given file. If a fatal error is encountered (such as a
	 *  permission problem), than this method should throw a FormatException. If the export can
	 *  proceed, but information might be lost in the process, than it should call
	 *  addWarning() with a translated, useful description of the issue.
	 */
	virtual void doExport() = 0;
	
	
protected:
	/// The input / output stream
	QIODevice* stream;
	
	/// The Map to import or export
	const Map* map;
	
	/// The MapView to import or export
	const MapView* view;
	
};


// ### ImportExport inline code ###

inline
const std::vector< QString >& ImportExport::warnings() const
{
	return warn;
}

inline
void ImportExport::addWarning(const QString& str)
{
	warn.push_back(str);
}

inline
void ImportExport::setOption(const QString& name, const QVariant& value)
{
	options[name] = value;
}




// ### Importer inline code ###

inline
const std::vector< ImportAction >& Importer::actions() const
{
	return act;
}

inline
void Importer::addAction(const ImportAction& action)
{
	act.push_back(action);
}


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_IMPORT_EXPORT_H
