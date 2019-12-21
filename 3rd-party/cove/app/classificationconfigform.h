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

#ifndef COVE_CLASSIFICATIONCONFIGFORM_H
#define COVE_CLASSIFICATIONCONFIGFORM_H

#include <QDialog>

#include "ui_classificationconfigform.h"

namespace cove {
namespace classificationconfigform_private {
class QDoubleInfValidator : public QDoubleValidator
{
	Q_OBJECT

public:
	QDoubleInfValidator(QObject* parent);
	QDoubleInfValidator(double bottom, double top, int decimals,
						QObject* parent);
	virtual QValidator::State validate(QString& input, int& pos) const;
};
} // classificationconfigform_private

class ClassificationConfigForm : public QDialog
{
	Q_OBJECT

	const QIntValidator positiveIntegerValid;
	const QDoubleValidator double01Valid;
	const classificationconfigform_private::QDoubleInfValidator double1InfValid;
	Ui::ClassificationConfigForm ui;

public:
	double initAlpha;
	double minAlpha;
	double Q;
	int E;
	int learnMethod;
	int colorSpace;
	double p;

	ClassificationConfigForm(QWidget* parent = 0);
	void accept();
	void setValues();

public slots:
	void on_learningMethodComboBox_activated(int);
};
} // cove

#endif
