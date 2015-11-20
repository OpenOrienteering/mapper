/*
 *    Copyright 2012, 2013 Kai Pastor, Thomas Schöps
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

#ifndef _OPENORIENTEERING_ABOUT_DIALOG_H_
#define _OPENORIENTEERING_ABOUT_DIALOG_H_

#include <QDialog>


/**
 * A dialog which shows information about Mapper and its components.
 */
class AboutDialog : public QDialog
{
Q_OBJECT
public:
	/** Construct a new About Dialog. */
	AboutDialog(QWidget* parent = NULL);
	
	/** Returns the basic information about this software. */
	static QString about();
	
	/** Returns the license text. */
	static QString licenseText();
	
	/** Returns information about additional components. */
	static QString additionalInformation();
	
signals:
	/** This signals is emitted when the user clicks a link. */
	void linkActivated(QString link);
};

#endif
