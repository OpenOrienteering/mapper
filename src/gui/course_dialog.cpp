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


#include "course_dialog.h"

#include <Qt>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "gui/util_gui.h"
#include "fileformats/course_export.h"


namespace OpenOrienteering {

CourseDialog::~CourseDialog() = default;

CourseDialog::CourseDialog(const CourseExport& course, QWidget* parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
, course{course}
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Event and course details"));
	
	auto* form_layout = new QFormLayout();
	
	event_name_edit = new QLineEdit();
	form_layout->addRow(tr("Event name:"), event_name_edit);
	
	course_name_edit = new QLineEdit();
	form_layout->addRow(tr("Course name:"), course_name_edit);
	
	first_code_spinbox = Util::SpinBox::create(1, 901);
	form_layout->addRow(tr("First code number:"), first_code_spinbox);

    start_symbol_code_edit = new QLineEdit();
    form_layout->addRow(tr("Start symbol code:"), start_symbol_code_edit);

    finish_symbol_code_edit = new QLineEdit();
    form_layout->addRow(tr("Finish symbol code:"), finish_symbol_code_edit);

    QRegularExpressionValidator* validator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[0-9.]*$")), this);
    start_symbol_code_edit->setValidator(validator);
    finish_symbol_code_edit->setValidator(validator);

	button_box = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	
	auto layout = new QVBoxLayout();
	layout->addLayout(form_layout, 1);
	layout->addWidget(button_box, 0);
	setLayout(layout);
	
	connect(button_box, &QDialogButtonBox::accepted, this, &CourseDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &CourseDialog::reject);
	updateWidgets();
}


QString CourseDialog::eventName() const
{
	return event_name_edit->text();
}

QString CourseDialog::courseName() const
{
	return course_name_edit->text();
}

QString CourseDialog::startSymbolCode() const
{
    return start_symbol_code_edit->text();
}

QString CourseDialog::finishSymbolCode() const
{
    return finish_symbol_code_edit->text();
}

int CourseDialog::firstCodeNumber() const
{
	return first_code_spinbox->value();
}

void CourseDialog::updateWidgets()
{
	event_name_edit->setText(course.eventName());
	course_name_edit->setText(course.courseName());
	//first_code_spinbox->setValue(course.firstCode());
    start_symbol_code_edit->setText(course.startSymbol());
    finish_symbol_code_edit->setText(course.finishSymbol());
}


}  // namespace OpenOrienteering
