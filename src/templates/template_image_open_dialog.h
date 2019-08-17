/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2012-2017 Kai Pastor
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


#ifndef OPENORIENTEERING_TEMPLATE_IMAGE_OPEN_DIALOG_H
#define OPENORIENTEERING_TEMPLATE_IMAGE_OPEN_DIALOG_H

#include <QDialog>
#include <QObject>
#include <QString>

class QLineEdit;
class QPushButton;
class QRadioButton;
class QWidget;

namespace OpenOrienteering {

class TemplateImage;


/**
 * Initial setting dialog when opening a raster image as template,
 * asking for how to position the image.
 * 
 * \todo Move this class to separate files.
 */
class TemplateImageOpenDialog : public QDialog
{
Q_OBJECT
public:
	TemplateImageOpenDialog(TemplateImage* templ, QWidget* parent);
	
	double getMpp() const;
	bool isGeorefRadioChecked() const;
	
protected slots:
	void radioClicked();
	void setOpenEnabled();
	void doAccept();
	
private:
	QRadioButton* georef_radio;
	QRadioButton* mpp_radio;
	QRadioButton* dpi_radio;
	QLineEdit* mpp_edit;
	QLineEdit* dpi_edit;
	QLineEdit* scale_edit;
	QPushButton* open_button;
	
	TemplateImage* templ;
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_TEMPLATE_IMAGE_OPEN_DIALOG_H
