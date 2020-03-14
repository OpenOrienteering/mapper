/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 *
 * This file is part of CoVe 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COVE_SETTINGS_H
#define COVE_SETTINGS_H

#include <vector>

#include <QRgb>
#include <QString>

namespace cove {
class Settings
{
	struct DoubleParam
	{
		QString name;
		double value;
	};
	std::vector<DoubleParam> doubleTab;
	struct ColorParam
	{
		QRgb color;
		QString comment;
	};
	std::vector<ColorParam> colorsTab;

public:
	Settings();
	double getDouble(const QString& attname) const;
	int getInt(const QString& attname) const;
	std::vector<QRgb> getInitColors(std::vector<QString>& comments) const;
	std::vector<QRgb> getInitColors() const;
	bool setDouble(const QString& attname, double attvalue);
	bool setInt(const QString& attname, int attvalue);
	bool setInitColors(const std::vector<QRgb>& clrs,
	                   const std::vector<QString>& comments);
};
} // cove

#endif
