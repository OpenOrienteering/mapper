/*
 *    Copyright 2016-2020 Kai Pastor
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

#include "gdal_manager.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
// IWYU pragma: no_include <type_traits>

#include <cpl_conv.h>
#include <gdal.h>
// IWYU pragma: no_include <gdal_version.h>

#include <QtGlobal>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QLatin1String>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "gdal/gdal_extensions.h"
#include "util/backports.h"  // IWYU pragma: keep


namespace OpenOrienteering {

class GdalManager::GdalManagerPrivate
{
public:
	const QString gdal_manager_group{ QStringLiteral("GdalManager") };
	const QString gdal_configuration_group{ QStringLiteral("GdalConfiguration") };

	// Enabled formats
	const QString gdal_gpx_key{ QStringLiteral("gpx") };
	
	// Template display options
	const QString gdal_hatch_key{ QStringLiteral("area_hatching") };
	const QString gdal_baseline_key{ QStringLiteral("baseline_view") };
	
	// Export options
	const QString ogr_one_layer_per_symbol_key{ QStringLiteral("per_symbol_layer") };
	
	
	using ExtensionList = std::vector<QByteArray>;
	
	
	GdalManagerPrivate()
	: dirty{ true }
	{
		GDALAllRegister();

		// Prefer LIBMKL driver to the KML driver if available
		if (GDALGetDriverByName("LIBKML") != nullptr)
			GDALDeregisterDriver(GDALGetDriverByName("KML"));
	}
	
	GdalManagerPrivate(const GdalManagerPrivate&) = delete;
	GdalManagerPrivate(GdalManagerPrivate&&) = delete;
	GdalManagerPrivate& operator=(const GdalManagerPrivate&) = delete;
	GdalManagerPrivate&& operator=(GdalManagerPrivate&&) = delete;
	
	void configure()
	{
		if (dirty)
			update();
	}
	
	
	QVariant settingsValue(const QString& key, const QVariant& default_value) const
	{
		QSettings settings;
		settings.beginGroup(gdal_manager_group);
		return settings.value(key, default_value);
	}

	void setSettingsValue(const QString& key, const QVariant& value)
	{
		QSettings settings;
		settings.beginGroup(gdal_manager_group);
		settings.setValue(key, value);
	}
	
	
	void setFormatEnabled(GdalManager::FileFormat format, bool enabled)
	{
		QString key;
		switch (format)
		{
		case GdalManager::GPX:
			key = gdal_gpx_key;
			break;
			
		}
		setSettingsValue(key, enabled);
		dirty = true;
	}
	
	bool isFormatEnabled(FileFormat format) const
	{
		QString key;
		bool default_value = true;
		switch (format)
		{
		case GdalManager::GPX:
			key = gdal_gpx_key;
			default_value = false;
			break;
		}
		return settingsValue(key, default_value).toBool();
	}

	void setExportOptionEnabled(GdalManager::ExportOption option, bool enabled)
	{
		QString key;
		switch (option)
		{
		case GdalManager::OneLayerPerSymbol:
			key = ogr_one_layer_per_symbol_key;
			break;
		}
		QSettings settings;
		settings.beginGroup(gdal_manager_group);
		settings.setValue(key, QVariant{ enabled });
		dirty = true;
	}

	bool isExportOptionEnabled(GdalManager::ExportOption option) const
	{
		QString key;
		switch (option)
		{
		case GdalManager::OneLayerPerSymbol:
			key = ogr_one_layer_per_symbol_key;
			break;
		}
		QSettings settings;
		settings.beginGroup(gdal_manager_group);
		return settings.value(key, QVariant{ false }).toBool();
	}
	
	const ExtensionList& supportedRasterExtensions() const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		return enabled_raster_import_extensions; 
	}
	
	const ExtensionList& supportedVectorImportExtensions() const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		return enabled_vector_import_extensions;
	}

	const ExtensionList& supportedVectorExportExtensions() const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		return enabled_vector_export_extensions;
	}
	
	QStringList parameterKeys() const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		return applied_parameters;
	}
	
	QString parameterValue(const QString& key) const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		QSettings settings;
		settings.beginGroup(gdal_configuration_group);
		return settings.value(key).toString();
	}

	void setParameterValue(const QString& key, const QString& value)
	{
		QSettings settings;
		settings.beginGroup(gdal_configuration_group);
		settings.setValue(key, QVariant{ value });
		dirty = true;
	}
	
	void unsetParameter(const QString& key)
	{
		QSettings settings;
		settings.beginGroup(gdal_configuration_group);
		settings.remove(key);
		dirty = true;
	}
	
private:
	static void copyExtensions(QByteArray const& extension_cstring, ExtensionList& extension_list)
	{
		for (auto pos = 0; pos >= 0; )
		{
			auto start = pos ? pos + 1 : 0;
			pos = extension_cstring.indexOf(' ', start);
			auto extension = extension_cstring.mid(start, pos - start);
			if (extension.isEmpty())
				continue;
			extension_list.emplace_back(extension);
		}
	}
	
	static void prefixDuplicates(ExtensionList const& primary_list, ExtensionList& secondary_list, char const* prefix)
	{
		for (auto& extension : secondary_list)
		{
			if (std::find(begin(primary_list), end(primary_list), extension) != end(primary_list))
				extension.prepend(prefix);
		}
	}
	
	static void removeExtension(ExtensionList& list, const char* extension)
	{
		list.erase(std::remove(begin(list), end(list), extension), end(list));
	}
	
	void updateExtensions(QSettings& settings)
	{
		static auto const qimagereader_extensions = gdal::qImageReaderExtensions<ExtensionList>();
		
		auto count = GDALGetDriverCount();
		enabled_raster_import_extensions.clear();
		enabled_raster_import_extensions.reserve(std::size_t(count));
		enabled_vector_import_extensions.clear();
		enabled_vector_import_extensions.reserve(std::size_t(count));
		enabled_vector_export_extensions.clear();
		enabled_vector_export_extensions.reserve(std::size_t(count));
		
		// Register extensions, either directly (enabled_*), or staged for ambiguity check (secondary_*)
		for (auto i = 0; i < count; ++i)
		{
			auto driver_data = GDALGetDriver(i);
			auto cap_raster = GDALGetMetadataItem(driver_data, GDAL_DCAP_RASTER, nullptr);
			auto cap_vector = GDALGetMetadataItem(driver_data, GDAL_DCAP_VECTOR, nullptr);
			auto cap_open = GDALGetMetadataItem(driver_data, GDAL_DCAP_OPEN, nullptr);
			auto cap_create = GDALGetMetadataItem(driver_data, GDAL_DCAP_CREATE, nullptr);
			auto extensions_raw = GDALGetMetadataItem(driver_data, GDAL_DMD_EXTENSIONS, nullptr);
			auto extensions = QByteArray::fromRawData(extensions_raw, int(qstrlen(extensions_raw)));

			if (qstrcmp(cap_raster, "YES") == 0)
			{
				if (qstrcmp(cap_open, "YES") == 0)
					copyExtensions(extensions, enabled_raster_import_extensions);
			}

			if (qstrcmp(cap_vector, "YES") == 0)
			{
				if (qstrcmp(cap_open, "YES") == 0)
					copyExtensions(extensions, enabled_vector_import_extensions);
				
				if (qstrcmp(cap_create, "YES") == 0)
					copyExtensions(extensions, enabled_vector_export_extensions);
			}
		}
		
		// Handle GDAL/OGR activation settings, before checking ambiguity
		settings.beginGroup(gdal_manager_group);
		if (!settings.value(gdal_gpx_key, false).toBool())
		{
			removeExtension(enabled_vector_import_extensions, "gpx");
		}
		settings.endGroup();
		
		// This block is duplicated in mapper_gdal_info.cpp dumpGdalDrivers().
		prefixDuplicates(qimagereader_extensions, enabled_raster_import_extensions, "raster.");
	}
	
	void updateConfig(QSettings& settings)
	{
		settings.beginGroup(gdal_configuration_group);
		
		// Using osmconf.ini to detect a directory with data from gdal. The
		// data:/gdal directory will always exist, due to mapper-osmconf.ini.
		auto osm_conf_ini = QFileInfo(QLatin1String("data:/gdal/osmconf.ini"));
		if (osm_conf_ini.exists())
		{
			auto gdal_data = osm_conf_ini.absolutePath();
			Q_ASSERT(!gdal_data.contains(QStringLiteral("data:")));
			// The user may overwrite this default in the settings.
			CPLSetConfigOption("GDAL_DATA", QDir::toNativeSeparators(gdal_data).toLocal8Bit());
		}
		
		const char* defaults[][2] = {
		    { "CPL_DEBUG",               "OFF" },
		    { "USE_PROJ_480_FEATURES",   "YES" },
		    { "OSM_USE_CUSTOM_INDEXING", "NO"  },
		    { "GPX_ELE_AS_25D",          "YES" },
		};
		for (const auto* setting : defaults)
		{
			const auto key = QString::fromLatin1(setting[0]);
			if (!settings.contains(key))
			{
				settings.setValue(key, QVariant{QLatin1String(setting[1])});
			}
		}
		
		osm_conf_ini = QFileInfo(QLatin1String("data:/gdal/mapper-osmconf.ini"));
		if (osm_conf_ini.exists())
		{
			auto osm_conf_ini_path = QDir::toNativeSeparators(osm_conf_ini.absoluteFilePath()).toLocal8Bit();
			auto key = QString::fromLatin1("OSM_CONFIG_FILE");
			auto update_settings = !settings.contains(key);
			if (!update_settings)
			{
				auto current = settings.value(key).toByteArray();
				settings.beginGroup(QLatin1String("default"));
				auto current_default = settings.value(key).toByteArray();
				settings.endGroup();
				update_settings = (current == current_default && current != osm_conf_ini_path);
			}
			if (update_settings)
			{
				settings.setValue(key, osm_conf_ini_path);
				settings.beginGroup(QLatin1String("default"));
				settings.setValue(key, osm_conf_ini_path);
				settings.endGroup();
			}
		}
		
		auto new_parameters = settings.childKeys();
		new_parameters.sort();
		for (const auto& parameter : qAsConst(new_parameters))
		{
			CPLSetConfigOption(parameter.toLatin1().constData(), settings.value(parameter).toByteArray().constData());
		}
		for (const auto& parameter : qAsConst(applied_parameters))
		{
			if (!new_parameters.contains(parameter)
			    && parameter != QLatin1String{ "GDAL_DATA" })
			{
				CPLSetConfigOption(parameter.toLatin1().constData(), nullptr);
			}
		}
		applied_parameters.swap(new_parameters);
		settings.endGroup();
	}
		
	void update()
	{
		QSettings settings;
		updateExtensions(settings);
		updateConfig(settings);
		CPLFinderClean(); // force re-initialization of file finding tools
		dirty = false;
	}
	
	mutable bool dirty;
	
	mutable ExtensionList enabled_raster_import_extensions;

	mutable ExtensionList enabled_vector_import_extensions;

	mutable ExtensionList enabled_vector_export_extensions;
	
	mutable QStringList applied_parameters;
	
};



// ### GdalManager ###

GdalManager::GdalManager()
{
	static GdalManagerPrivate manager;
	p = &manager;
}

void GdalManager::configure()
{
	p->configure();
}


bool GdalManager::isAreaHatchingEnabled() const
{
	return p->settingsValue(p->gdal_hatch_key, false).toBool();
}

void GdalManager::setAreaHatchingEnabled(bool enabled)
{
	p->setSettingsValue(p->gdal_hatch_key, enabled);
}

bool GdalManager::isBaselineViewEnabled() const
{
	return p->settingsValue(p->gdal_baseline_key, false).toBool();
}

void GdalManager::setBaselineViewEnabled(bool enabled)
{
	p->setSettingsValue(p->gdal_baseline_key, enabled);
}


void GdalManager::setFormatEnabled(GdalManager::FileFormat format, bool enabled)
{
	p->setFormatEnabled(format, enabled);
}

bool GdalManager::isFormatEnabled(GdalManager::FileFormat format) const
{
	return p->isFormatEnabled(format);
}

void GdalManager::setExportOptionEnabled(GdalManager::ExportOption option, bool enabled)
{
	return p->setExportOptionEnabled(option, enabled);
}

bool GdalManager::isExportOptionEnabled(GdalManager::ExportOption option) const
{
	return p->isExportOptionEnabled(option);
}

const std::vector<QByteArray>&GdalManager::supportedRasterExtensions() const
{
	return p->supportedRasterExtensions();
}

const std::vector<QByteArray>& GdalManager::supportedVectorImportExtensions() const
{
	return p->supportedVectorImportExtensions();
}

const std::vector<QByteArray>& GdalManager::supportedVectorExportExtensions() const
{
	return p->supportedVectorExportExtensions();
}

QStringList GdalManager::parameterKeys() const
{
	return p->parameterKeys();
}

QString GdalManager::parameterValue(const QString& key) const
{
	return p->parameterValue(key);
}

void GdalManager::setParameterValue(const QString& key, const QString& value)
{
	p->setParameterValue(key, value);
}

void GdalManager::unsetParameter(const QString& key)
{
	p->unsetParameter(key);
}


}  // namespace OpenOrienteering
