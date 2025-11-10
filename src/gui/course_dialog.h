/*
 *    Copyright 2021 Kai Pastor
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


#ifndef OPENORIENTEERING_COURSE_DIALOG_H
#define OPENORIENTEERING_COURSE_DIALOG_H

#include <QtGlobal>
#include <QDialog>
#include <QObject>
#include <QString>

class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class CourseExport;


/**
 * A dialog to provide extra information for course export.
 */
class CourseDialog : public QDialog
{
Q_OBJECT
public:
	~CourseDialog() override;
	
	CourseDialog(const CourseExport& course_export, QWidget* parent);
	
	QString eventName() const;
	
	QString courseName() const;

    QString startSymbolCode() const;

    QString finishSymbolCode() const;
	
	int firstCodeNumber() const;
	
protected:
	void updateWidgets();
	
private:
	const CourseExport& course_export;
	QLineEdit* event_name_edit;
	QLineEdit* course_name_edit;
	QSpinBox*  first_code_spinbox;
    QLineEdit* start_symbol_code_edit;
    QLineEdit* finish_symbol_code_edit;
	QDialogButtonBox* button_box;
	
	Q_DISABLE_COPY(CourseDialog)
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_COURSE_DIALOG_H
