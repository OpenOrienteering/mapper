/*
 *    Copyright 2023 Matthias Kühlewein
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

#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QVariant>

class QString;
class QJsonParseError;

namespace OpenOrienteering {

class JSONConfigurationObject
{
public:
	friend class JSONConfiguration;
	
	JSONConfigurationObject() = default;
	JSONConfigurationObject(const JSONConfigurationObject&) = default;
	bool operator!= (const JSONConfigurationObject& other) const { return json_object != other.json_object; };
	
	void setDefaultValues();
	bool readJSONFile(const QString& filename);
	bool writeJSONFile(const QString& filename);
	bool JSONfromText(const QString& text, QJsonParseError *error = nullptr);
	QString getText() const;
	bool IsValid() const;
	QJsonObject& getJSONObject() { return json_object; };
	
private:
	QJsonObject json_object;
	QJsonDocument json_doc;
};


/**
 *
 */
class JSONConfiguration
{
private:
	JSONConfiguration();
	
public:
	static JSONConfiguration& getInstance();
	JSONConfiguration(JSONConfiguration const&) = delete;
	void operator=(JSONConfiguration const&) = delete;
	
	JSONConfigurationObject* getDefault() const {return default_config;}
	static QVariant getConfigValue(const QString& key) { return getInstance().getValue(key); }
	QVariant getValue(const QString& key) const;
	const QString& getJSONFilename() const { return json_filename; }
	void setJSONFilename(QString& filename) { json_filename = filename; }
	void setJSONConfig(JSONConfigurationObject& config);
	JSONConfigurationObject* getLoadedConfig() const { return loaded_config; }
	bool isLoadedConfigValid() const { return loaded_config && loaded_config->IsValid(); }
	void makeActiveConfig();
	void loadConfig(const QString& filename);
	
private:
	JSONConfigurationObject* default_config;	// the default configuration set
	JSONConfigurationObject* active_config;		// the configuration set used by Mapper operations
	JSONConfigurationObject* loaded_config;		// the configuration retrieved from storage or from user configuration
	QString json_filename;
};

}  // namespace Openorienteering

#endif // JSON_CONFIG_H
