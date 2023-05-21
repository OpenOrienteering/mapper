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

#include "json_config.h"
#include "settings.h"

#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QVariant>


namespace OpenOrienteering {

void JSONConfigurationObject::setDefaultValues()
{
	json_object.insert(QString::fromLatin1("DeclLookupKey"), QJsonValue(QString::fromLatin1("zNEw7")));
	json_object.insert(QString::fromLatin1("DashPointsAsDefault"), QJsonValue(true));
}

bool JSONConfigurationObject::readJSONFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return false;
	
	auto json_data = file.read(2000);	// arbitrary limit
	file.close();
	json_doc = QJsonDocument::fromJson(json_data);
	if (!json_doc.isNull() && json_doc.isObject())
	{
		json_object = QJsonObject(json_doc.object());
		return true;
	}
	return false;
}

bool JSONConfigurationObject::writeJSONFile(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	
	file.write(json_doc.toJson());
	file.close();
	return true;
}

QString JSONConfigurationObject::getText() const
{
	auto json_doc = new QJsonDocument(json_object);
	auto json_text = QString::fromStdString(json_doc->toJson().toStdString());
	return json_text;
}

bool JSONConfigurationObject::JSONfromText(const QString& text, QJsonParseError *error)
{
	json_doc = QJsonDocument::fromJson(text.toUtf8(), error);
	if (json_doc.isNull())
		return false;
	json_object = QJsonObject(json_doc.object());
	return true;
}

bool JSONConfigurationObject::IsValid() const
{
	return !json_object.isEmpty() && !QJsonDocument(json_object).isNull();
}

JSONConfiguration::JSONConfiguration() :
  active_config(nullptr)
, loaded_config(nullptr)
{
	default_config = new JSONConfigurationObject();
	default_config->setDefaultValues();
	auto filename = Settings::getInstance().getSetting(Settings::General_JSONConfigurationFile).toString();
	if (!filename.isEmpty())
	{
		loadConfig(filename);
	}
	makeActiveConfig();
}

JSONConfiguration& JSONConfiguration::getInstance()
{
	static JSONConfiguration instance;
	return instance;
}

void JSONConfiguration::loadConfig(const QString& filename)
{
	if (loaded_config)
		delete loaded_config;
	loaded_config = new JSONConfigurationObject();
	if (loaded_config->readJSONFile(filename))
		json_filename = filename;
	makeActiveConfig();
}

void JSONConfiguration::setJSONConfig(JSONConfigurationObject& config)
{
	if (loaded_config)
		delete loaded_config;
	loaded_config = new JSONConfigurationObject(config);
	makeActiveConfig();
}

void JSONConfiguration::makeActiveConfig()
{
	if (active_config)
		delete active_config;
	if (!loaded_config || !loaded_config->IsValid())
		active_config = new JSONConfigurationObject(*default_config);
	else
	{
		active_config = new JSONConfigurationObject();
		const auto& json_obj = default_config->json_object;
		for (auto it = json_obj.constBegin(); it != json_obj.constEnd(); ++it)
		{
			if (loaded_config->json_object.contains(it.key()) && 
				loaded_config->json_object.value(it.key()).type() ==  default_config->json_object.value(it.key()).type())
				active_config->json_object.insert(it.key(), loaded_config->json_object.value(it.key()));
			else active_config->json_object.insert(it.key(), it.value());
		}
	}
}

QVariant JSONConfiguration::getValue(const QString& key) const
{
	Q_ASSERT(active_config && active_config->json_object.contains(key));
	
	return active_config->json_object.value(key).toVariant();
}

}  // namespace OpenOrienteering
