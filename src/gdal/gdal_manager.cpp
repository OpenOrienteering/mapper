/*
 *    Copyright 2016-2018 Kai Pastor
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

#include <cstddef>

#include <cpl_conv.h>
#include <gdal.h> // IWYU pragma: keep
#include <ogr_api.h>

#include <QtGlobal>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QLatin1String>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "util/backports.h"


namespace OpenOrienteering {

class GdalManager::GdalManagerPrivate
{
public:
	const QString gdal_manager_group{ QStringLiteral("GdalManager") };
	const QString gdal_configuration_group{ QStringLiteral("GdalConfiguration") };
	const QString gdal_dxf_key{ QStringLiteral("dxf") };
	const QString gdal_gpx_key{ QStringLiteral("gpx") };
	const QString gdal_osm_key{ QStringLiteral("osm") };
	const QString gdal_hatch_key{ QStringLiteral("area_hatching") };
	const QString gdal_baseline_key{ QStringLiteral("baseline_view") };
	
	GdalManagerPrivate()
	: dirty{ true }
	{
		// GDAL 2.0: GDALAllRegister();
		OGRRegisterAll();
	}
	
	GdalManagerPrivate(const GdalManagerPrivate&) = delete;
	
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
		case GdalManager::DXF:
			key = gdal_dxf_key;
			break;
			
		case GdalManager::GPX:
			key = gdal_gpx_key;
			break;
			
		case GdalManager::OSM:
			key = gdal_osm_key;
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
		case GdalManager::DXF:
			key = gdal_dxf_key;
			break;
			
		case GdalManager::GPX:
			key = gdal_gpx_key;
			default_value = false;
			break;
			
		case GdalManager::OSM:
			key = gdal_osm_key;
			break;
		}
		return settingsValue(key, default_value).toBool();
	}
	
	const std::vector<QByteArray>& supportedRasterExtensions() const
	{
		/// \todo
		static std::vector<QByteArray> ret;
		return ret; 
	}
	
	const std::vector<QByteArray>& supportedVectorExtensions() const
	{
		if (dirty)
			const_cast<GdalManagerPrivate*>(this)->update();
		return enabled_vector_extensions;
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
	void update()
	{
		QSettings settings;
		
#ifdef GDAL_DMD_EXTENSIONS
		// GDAL >= 2.0
		settings.beginGroup(gdal_manager_group);
		auto count = GDALGetDriverCount();
		enabled_vector_extensions.clear();
		enabled_vector_extensions.reserve(std::size_t(count));
		for (auto i = 0; i < count; ++i)
		{
			auto driver_data = GDALGetDriver(i);
			auto type = GDALGetMetadataItem(driver_data, GDAL_DCAP_VECTOR, nullptr);
			if (qstrcmp(type, "YES") != 0)
				continue;
			
			// Skip write-only drivers.
			auto cap_open = GDALGetMetadataItem(driver_data, GDAL_DCAP_OPEN, nullptr);
			if (qstrcmp(cap_open, "YES") != 0)
				continue;
			
			auto extensions_raw = GDALGetMetadataItem(driver_data, GDAL_DMD_EXTENSIONS, nullptr);
			auto extensions = QByteArray::fromRawData(extensions_raw, int(qstrlen(extensions_raw)));
			for (auto pos = 0; pos >= 0; )
			{
				auto start = pos ? pos + 1 : 0;
				pos = extensions.indexOf(' ', start);
				auto extension = extensions.mid(start, pos - start);
				if (extension.isEmpty())
					continue;
				if (extension == "dxf" && !settings.value(gdal_dxf_key, true).toBool())
					continue;
				if (extension == "gpx" && !settings.value(gdal_gpx_key, false).toBool())
					continue;
				if (extension == "osm" && !settings.value(gdal_osm_key, true).toBool())
					continue;
				enabled_vector_extensions.emplace_back(extension);
			}
		}
		settings.endGroup();
#else
		// GDAL < 2.0 does not provide the supported extensions 
		static const std::vector<QByteArray> default_extensions = {
		    "shp", "dbf",
		    /* "dxf", */
		    /* "gpx", */
		    /* "osm", */ "pbf",
		};
		enabled_vector_extensions.reserve(default_extensions.size() + 3);
		enabled_vector_extensions = default_extensions;
		
		settings.beginGroup(gdal_manager_group);
		if (settings.value(gdal_dxf_key, true).toBool())
			enabled_vector_extensions.push_back("dxf");
		if (settings.value(gdal_gpx_key, false).toBool())
			enabled_vector_extensions.push_back("gpx");
		if (settings.value(gdal_osm_key, true).toBool())
			enabled_vector_extensions.push_back("osm");
		settings.endGroup();
#endif
		
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
		
		settings.beginGroup(gdal_configuration_group);
		
		const char* defaults[][2] = {
		    { "CPL_DEBUG",               "OFF" },
		    { "USE_PROJ_480_FEATURES",   "YES" },
		    { "OSM_USE_CUSTOM_INDEXING", "NO"  },
		    { "GPX_ELE_AS_25D",          "YES" },
		};
		for (const auto setting : defaults)
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
		
		CPLFinderClean(); // force re-initialization of file finding tools
		dirty = false;
	}
	
	mutable bool dirty;
	
	mutable std::vector<QByteArray> enabled_vector_extensions;
	
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

const std::vector<QByteArray>&GdalManager::supportedRasterExtensions() const
{
	return p->supportedRasterExtensions();
}

const std::vector<QByteArray>& GdalManager::supportedVectorExtensions() const
{
	return p->supportedVectorExtensions();
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
