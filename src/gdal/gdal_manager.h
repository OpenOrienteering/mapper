/*
 *    Copyright 2016, 2018 Kai Pastor
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

#ifndef OPENORIENTEERING_GDAL_MANAGER_H
#define OPENORIENTEERING_GDAL_MANAGER_H


#include <vector>

class QByteArray;
class QString;
class QStringList;


namespace OpenOrienteering {

/**
 * A utility class which takes care of GDAL settings and options.
 * 
 * This class provides lists of extensions supported via GDAL in Mapper.
 * It sets and updates GDAL configuration parameters from Mapper's settings.
 * 
 * There is no need to keep objects of this class for an extended life time:
 * instantiation is cheap; the actual state is shared and retained.
 */
class GdalManager
{
private:
	class GdalManagerPrivate;
	
	GdalManagerPrivate* p;
	
public:
	enum FileFormat
	{
		DXF,
		GPX,
		OSM
	};

	enum ExportOption
	{
		OneLayerPerSymbol
	};
	
	/**
	 * Constructs a new manager object.
	 */
	GdalManager();
	
	/**
	 * Sets the GDAL configuration from Mapper's defaults and settings.
	 */
	void configure();
	
	
	/**
	 * Returns the area hatching display setting.
	 */
	bool isAreaHatchingEnabled() const;
	
	/**
	 * Sets the area hatching template display setting.
	 */
	void setAreaHatchingEnabled(bool enabled);
	
	
	/**
	 * Returns the baseline view template display setting.
	 */
	bool isBaselineViewEnabled() const;
	
	/**
	 * Sets the baseline view template display setting.
	 */
	void setBaselineViewEnabled(bool enabled);
	
	
	/**
	 * Enables or disables handling of a particular file format by GDAL/OGR.
	 */
	void setFormatEnabled(FileFormat format, bool enabled);
	
	/**
	 * Returns if GDAL/OGR will handle a particular file format.
	 */
	bool isFormatEnabled(FileFormat format) const;
	

	/**
	 * Enables or disables an OGR export option
	 */
	void setExportOptionEnabled(ExportOption option, bool enabled);
	
	/**
	 * Returns if an OGR export option is enabled
	 */
	bool isExportOptionEnabled(ExportOption option) const;


	/**
	 * Returns the file name extensions for supported raster formats.
	 */
	const std::vector<QByteArray>& supportedRasterExtensions() const;
	
	/**
	 * Returns the file name extensions for supported vector formats.
	 */
	const std::vector<QByteArray>& supportedVectorImportExtensions() const;

	/**
	 * Returns the file name extensions for supported vector export formats.
	 */
	const std::vector<QByteArray>& supportedVectorExportExtensions() const;
	
	
	/**
	 * Returns the list of GDAL configuration parameters.
	 */
	QStringList parameterKeys() const;

	/**
	 * Returns a GDAL configuration parameter value.
	 */
	QString parameterValue(const QString& key) const;
	
	/**
	 * Sets a GDAL configuration parameter value.
	 */
	void setParameterValue(const QString& key, const QString& value);
	
	/**
	 * Unsets a GDAL configuration parameter value.
	 */
	void unsetParameter(const QString& key);
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_GDAL_MANAGER_H
