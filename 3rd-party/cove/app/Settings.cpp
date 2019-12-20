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

#include <cmath>
#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "app/Settings.h"

namespace cove {
//@{
//! \ingroup gui

/*! \class Settings
  \brief Holds the settings.
  */

/*! \struct Settings::DoubleParam */
/*! \var const char* Settings::DoubleParam::name
 */
/*! \var double Settings::DoubleParam::value
 */
/*! \var QDomDocument Settings::storage
 XML storage of the ettings. */

/*! Constructor, creates root element and fills defaults from \a doubletab to
 * \a storage */
Settings::Settings()
{
	doubleTab = {{"E", 100000},
				 {"q", 0.5},
				 {"initAlpha", 0.1},
				 {"minAlpha", 1e-6},
				 {"learnMethod", 0},
				 {"colorSpace", 0},
				 {"p", INFINITY},
				 {"alphaStrategy", 0},
				 {"patternStrategy", 0},
				 {"nColors", 2},
				 {"initColorsSource", 1},
				 {"speckleSize", 5},
				 {"doConnections", 1},
				 {"joinDistance", 5},
				 {"simpleConnectionsOnly", 1},
				 {"distDirBalance", 0.5}};
}

/*! Returns double from storage.
  \param[in] attname Name of the variable to be retrieved.
  \return NaN in case the variable name does not exist */
double Settings::getDouble(const QString& attname) const
{
	for (auto& k : doubleTab)
		if (k.name == attname) return k.value;

	return std::nan("");
}

/*! Returns integer from storage.
  \param[in] attname Name of the variable to be retrieved.
  \return Zero in case the variable name does not exist */
int Settings::getInt(const QString& attname) const
{
	double d = getDouble(attname);
	return std::isfinite(d) ? d : 0;
}

/*! Returns QRgb table of initial colors stored in element initColors.
  \param[in,out] comments QString array reference where the comments will be
  stored.
  \return Array of QRgb, initial colors. Empty when node initColors does not
  exist. */
std::vector<QRgb> Settings::getInitColors(std::vector<QString>& comments) const
{
	std::vector<QRgb> clrs(colorsTab.size());
	comments.resize(colorsTab.size());

	for (auto i = 0U; i < colorsTab.size(); ++i)
	{
		clrs[i] = colorsTab[i].color;
		comments[i] = colorsTab[i].comment;
	}

	return clrs;
}

std::vector<QRgb> Settings::getInitColors() const
{
	std::vector<QString> comments;
	return getInitColors(comments);
}

/*! Sets double value in storage.
  \param[in] attname Name of the variable to be set.
  \param[in] attvalue Value associated with the name. */
bool Settings::setDouble(const QString& attname, double attvalue)
{
	for (auto& k : doubleTab)
	{
		if (k.name == attname)
		{
			k.value = attvalue;
			return true;
		}
	}

	return false;
}

/*! Sets integer value in storage.
  \param[in] attname Name of the variable to be set.
  \param[in] attvalue Value associated with the name. */
bool Settings::setInt(const QString& attname, int attvalue)
{
	return setDouble(attname, attvalue);
}

/*! Sets QRgb table of initial colors stored in element initColors.
  \param[in] clrs Array of QRgb, initial colors.
  \param[in] comments QString array with user comments. Ensuring that comments
				 array matches colors array in size is the caller's
  responsibility.
  \return True when the operation is successful. */
bool Settings::setInitColors(const std::vector<QRgb>& clrs,
							 const std::vector<QString>& comments)
{
	colorsTab.resize(clrs.size());

	for (auto i = 0U; i < clrs.size(); i++)
	{
		colorsTab[i].color = clrs[i];
		colorsTab[i].comment = comments[i];
	}

	return true;
}
} // cove

//@}
