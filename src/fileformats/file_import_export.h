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
	/**
	 * Creates a new importer or exporter with the given IO device.
	 */
	ImportExport(QIODevice* device = nullptr)
	: device_(device)
	{}
	
	ImportExport(const ImportExport&) = delete;
	ImportExport(ImportExport&&) = delete;
	
	/** Destroys an importer or exporter.
	 */
	virtual ~ImportExport();
	
	
	/**
	 * Returns true when the importer/exporter supports QIODevice.
	 * 
	 * The default implementation returns `true`. Some exporters or importers
	 * might wish to use other ways of accessing the output/input path.
	 * In these cases, the default implementation should be overwritten to
	 * return `false`.
	 */
	virtual bool supportsQIODevice() const noexcept;
	
	/**
	 * Returns the input device if it has been set or created.
	 */
	QIODevice* device() const noexcept { return device_; }
	
	/**
	 * Sets a custom input device to be used for import.
	 * 
	 * Not all importers may be able to use such devices.
	 * 
	 * \see ImportExport::supportQIODevice()
 	 */
	void setDevice(QIODevice* device);
	
	
	/**
	 * Sets an option in this importer or exporter.
	 */
	void setOption(const QString& name, const QVariant& value);
	
	/**
	 * Retrieves the value of an option in this instance.
	 * 
	 * The option's value must have been set before this function is called,
	 * either in the constructor, or later through setOption(). Otherwise a
	 * FormatException will be thrown.
	 */
	QVariant option(const QString& name) const;
	
	
	/**
	 * Adds a string to the current list of warnings.
	 * 
	 * The provided message should be translated.
	 */
	void addWarning(const QString& str) { warnings_.emplace_back(str); }
	
	/**
	 * Returns the current list of warnings collected by this object.
	 */
	const std::vector<QString>& warnings() const noexcept { return warnings_; }
	
private:
	friend class Exporter;  // direct access to device_ in Exporter::doExport()
	friend class Importer;  // direct access to device_ in Importer::doImport()
	
	/// The input / output device
	QIODevice* device_;
	
	/// A list of options for the import/export
	QHash<QString, QVariant> options;
	
	/// A list of warnings
	std::vector<QString> warnings_;
};



/**
 * Base class for all importers.
 * 
 * An Importer has the following lifecycle:
 * 
 * 1. The Importer is constructed with the input path and with pointers to map
 *    and view. The constructor should also set default values for any options
 *    it will read. (ImportExport will throw an exception if the exporter reads
 *    an option that does not have a value.)
 * 2. setOption() can be called zero or more times to customize the options.
 *    setDevice() can be called to set a custom input device.
 * 3. doImport() will be called to perform the import.
 */
class Importer : public ImportExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::Importer)
	
public:
	/**
	 * Creates a new Importer with the given input path, map, and view.
	 */
	Importer(const QString& path, Map* map, MapView* view)
	: ImportExport(nullptr), path(path), map(map), view(view)
	{}
	
	/**
	 * Destroys this Importer.
	 */
	~Importer() override;
	
	
	/**
	 * Returns true if only symbols (and colors) are to be imported.
	 */
	bool loadSymbolsOnly() const noexcept { return load_symbols_only; }
	
	/**
	 * If set to true, importers shall import only symbols (and colors).
	 */
	void setLoadSymbolsOnly(bool value);
	
	
	/**
	 * Imports the map and view.
	 * 
	 * This is the function which is normally called to perform import.
	 * It must return false on errors and true on success. All error and
	 * warning messages must be logged in `ImportExport::warnings()`.
	 * 
	 * In case of an error, the last warning can be assumed to be the error
	 * message. (The list of warnings won't be empty then.)
	 * 
	 * If the importer supports input from QIODevice, this function will open
	 * a QFile with the given path and make it available via device().
	 */
	bool doImport();
	
	
protected:
	/**
	 * Event handler called from doImport() to prepare the import process.
	 * 
	 * This function is called only when opening the input succeeded. However,
	 * the input is not passed to this function by intention: It is meant to
	 * be used also by importers which do not use QIODevice.
	 * 
	 * Overriding functions must call the the default implementation.
	 */
	virtual void prepare();
	
	/**
	 * Actual implementation of the import.
	 * 
	 * This function shall read the input and populate the map and view from it.
	 * Importers which support input from QIODevice should use the device().
	 * They must not open or close this device.
	 * 
	 * If information might be lost in the process, a message shall be recorded
	 * by calling addWarning() with a translated, useful description of the
	 * issue. If a fatal error is encountered, then this method may either throw
	 * an exception, or log an error message with addWarning() and return false.
	 * The line between errors and warnings is somewhat of a judgement call on
	 * the part of the author, but generally an Importer should not succeed
	 * unless the map is populated sufficiently to be useful.
	 */
	virtual bool importImplementation() = 0;
	
	/**
	 * Event handler called from doImport() to complete to postprocess imported data.
	 * 
	 * Overriding functions must call the the default implementation early.
	 * It validates the object and symbols, and looks for templates. It may
	 * throw FileFormatException.
	 */
	virtual void validate();
	
	/**
	 * Event handler called from doImport() to complete to cleanup a failed
	 * import attempt.
	 * 
	 * Overriding functions must call the the default implementation.
	 */
	virtual void importFailed();
	
	
protected:
	/// The input path
	const QString path;
	
	/// The Map to import or export
	Map* const map;
	
	/// The MapView to import or export
	MapView* const view;
	
	
private:
	/// A flag which controls whether only symbols and colors are imported.
	bool load_symbols_only = false;
	
};


/**
 * Base class for all exporters.
 * 
 * An Exporter has the following lifecycle:
 *
 * 1. The constructor is called with a path, map and view. The path may be empty
 *    if the output device can be (and is) set later. The view may be nullptr.
 *    The constructor must also set default values for any options the exporter
 *    will read. (ImportExport will throw an exception if the exporter reads an
 *    option that has not been set.)
 * 2. setOption() can be called zero or more times to customize the options.
 * 3. doExport() will be called to perform the export.
 */
class Exporter : public ImportExport
{
	Q_DECLARE_TR_FUNCTIONS(OpenOrienteering::Exporter)
	
public:
	/**
	 * Creates a new Exporter with the given path, map, and view.
	 */
	Exporter(const QString& path, const Map* map, const MapView* view)
	: ImportExport(nullptr), path(path), map(map), view(view)
	{}
	
	/** Destroys the current Exporter.
	 */
	~Exporter() override;
	
	
	/**
	 * Exports the map and view.
	 * 
	 * This function calls exportImplementation() and catches exceptions
	 * thrown there. It returns false on errors and true on success.
	 * All error and warning messages are logged in ImportExport::warnings().
	 * In case of an error, the last warning can be assumed to be the error
	 * message. (The list of warnings won't be empty then.)
	 * 
	 * If the exporter supports input from QIODevice but a device hasn't been
	 * set yet, this function will open a QSaveFile with the given path, make it
	 * available via device(), and commit the changes if exportImplementation()
	 * returns successfully.
	 */
	bool doExport();
	
	
protected:
	/**
	 * Actual implementation of the export.
	 * 
	 * Exporters which support input from QIODevice shall use the device().
	 * They must not open or close the device.
	 * 
	 * If information might be lost in the export, a message shall be recorded
	 * by calling addWarning() with a translated, useful description of the
	 * issue. If a fatal error is encountered, then this method may either throw
	 * an exception, or log an error message with addWarning() and return false.
	 * 
	 * There is no default implementation.
	 */
	virtual bool exportImplementation() = 0;
	
	
	/// The output path
	const QString path;
	
	/// The Map to import or export
	const Map* const map;
	
	/// The MapView to import or export
	const MapView* const view;
	
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_IMPORT_EXPORT_H
