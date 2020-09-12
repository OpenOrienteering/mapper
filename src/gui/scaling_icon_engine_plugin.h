/*
 *    Copyright 2020 Kai Pastor
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


#ifndef OPENORIENTEERING_SCALING_ICON_ENGINE_PLUGIN_H
#define OPENORIENTEERING_SCALING_ICON_ENGINE_PLUGIN_H

#include <QIconEnginePlugin>
#include <QObject>
#include <QString>

class QIconEngine;

namespace OpenOrienteering {


/**
 * This plugin provides an icon engine that can scale up icons.
 * 
 * For most of its behaviour, this plugin and the icon engine it creates rely
 * on Qt's default icon engine. It merely acts as a proxy. This is achieved by
 * registering this plugin with high priority for "PNG", but using a delegate
 * created by Qt for "BMP". In this configuration, Qt will select this plugin
 * only for PNG files.
 */
class ScalingIconEnginePlugin : public QIconEnginePlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QIconEngineFactoryInterface"
	                  FILE "scaling_icon_engine_plugin.json")
	
public:
	~ScalingIconEnginePlugin() override;
	ScalingIconEnginePlugin(QObject *parent = nullptr);
	virtual QIconEngine *create(const QString& filename = {}) override;
};


}  // namespace OpenOrienteering

#endif // OPENORIENTEERING_SCALING_ICON_ENGINE_H
