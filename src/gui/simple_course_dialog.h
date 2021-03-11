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


#ifndef OPENORIENTEERING_SIMPLE_COURSE_DIALOG_H
#define OPENORIENTEERING_SIMPLE_COURSE_DIALOG_H

#include <QtGlobal>
#include <QDialog>
#include <QObject>
#include <QString>

class QDialogButtonBox;
class QLineEdit;
class QSpinBox;
class QWidget;

namespace OpenOrienteering {

class SimpleCourseExport;


/**
 * A dialog to provide extra information for simple course export.
 */
class SimpleCourseDialog : public QDialog
{
Q_OBJECT
public:
	~SimpleCourseDialog() override;
	
	SimpleCourseDialog(const SimpleCourseExport& simple_course, QWidget* parent);
	
	QString eventName() const;
	
	QString courseName() const;
	
	int firstCodeNumber() const;
	
protected:
	void updateWidgets();
	
private:
	const SimpleCourseExport& simple_course;
	QLineEdit* event_name_edit;
	QLineEdit* course_name_edit;
	QSpinBox*  first_code_spinbox;
	QDialogButtonBox* button_box;
	
	Q_DISABLE_COPY(SimpleCourseDialog)
};


}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_SIMPLE_COURSE_DIALOG_H
