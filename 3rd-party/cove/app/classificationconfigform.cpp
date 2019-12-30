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

#include "classificationconfigform.h"
#include "ui_classificationconfigform.h"

#include <cmath>
#include <climits>

#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QValidator>

class QWidget;

using cove::classificationconfigform_private::QDoubleInfValidator;

namespace cove {
//@{
//! \ingroup gui

/*! \class ClassificationConfigForm
  \brief Provides configuration dialog for classification options. */

/*! \var double ClassificationConfigForm::initAlpha
  Initial alpha for Kohonen learning.
  \sa ClassicAlphaGetter */
/*! \var double ClassificationConfigForm::minAlpha
  Minimal alpha for Kohonen learning.
  \sa ClassicAlphaGetter */
/*! \var double ClassificationConfigForm::Q
  Quotient for Kohonen learning.
  \sa ClassicAlphaGetter */
/*! \var int ClassificationConfigForm::E
  E for Kohonen learning.
  \sa ClassicAlphaGetter */
/*! \var int ClassificationConfigForm::learnMethod
  Learning method used.
  \sa Vectorizer::LearningMethod*/
/*! \var int ClassificationConfigForm::colorSpace
  Color space used.
  \sa Vectorizer::ColorSpace*/
/*! \var double ClassificationConfigForm::p
  Minkowski metric parameter.
  \sa MapColor
 */

/*! \class classificationconfigform_private::QDoubleInfValidator
  \brief Validator accepting doubles and 'inf' as input. */

QDoubleInfValidator::QDoubleInfValidator(QObject* parent)
	: QDoubleValidator(parent)
{
}

QDoubleInfValidator::QDoubleInfValidator(double bottom, double top,
										 int decimals, QObject* parent)
	: QDoubleValidator(bottom, top, decimals, parent)
{
}

//! Reimplemented validate to accept infinity.
QDoubleValidator::State QDoubleInfValidator::validate(QString& input,
													  int& pos) const
{
	static QString inf = "inf";
	static QString minusinf = "-inf";
	if (std::isinf(top()) && !std::signbit(top()))
	{
		if (input == inf) return Acceptable;
		if (inf.startsWith(input)) return Intermediate;
	}
	if (std::isinf(bottom()) && std::signbit(top()))
	{
		if (input == minusinf) return Acceptable;
		if (minusinf.startsWith(input)) return Intermediate;
	}
	return QDoubleValidator::validate(input, pos);
}

//! Assigns validators to LineEdit components.
ClassificationConfigForm::ClassificationConfigForm(QWidget* parent)
	: QDialog(parent)
	, positiveIntegerValid(0, INT_MAX, this)
	, double01Valid(0.0, 1.0, 15, this)
	, double1InfValid(1.0, INFINITY, 15, this)
{
	ui.setupUi(this);

	ui.initAlphaInput->setValidator(&double01Valid);
	ui.eInput->setValidator(&positiveIntegerValid);
	ui.qInput->setValidator(&double01Valid);
	ui.minAlphaInput->setValidator(&double01Valid);
	ui.pInput->setValidator(&double1InfValid);
}

//! Rewrites data from input components to data members.
void ClassificationConfigForm::accept()
{
	E = ui.eInput->text().toInt();
	Q = ui.qInput->text().toDouble();
	initAlpha = ui.initAlphaInput->text().toDouble();
	minAlpha = ui.minAlphaInput->text().toDouble();
	learnMethod = ui.learningMethodComboBox->currentIndex();
	colorSpace = ui.colorSpaceComboBox->currentIndex();
	bool ok;
	p = ui.pInput->text().toDouble(&ok);
	if (!ok && ui.pInput->text() == QString::fromLatin1("inf")) p = INFINITY;
	QDialog::accept();
}

//! Rewrites data from data members to input components.
void ClassificationConfigForm::setValues()
{
	ui.eInput->setText(QString::number(E));
	ui.qInput->setText(QString::number(Q));
	ui.minAlphaInput->setText(QString::number(minAlpha));
	ui.initAlphaInput->setText(QString::number(initAlpha));
	ui.learningMethodComboBox->setCurrentIndex(learnMethod);
	ui.colorSpaceComboBox->setCurrentIndex(colorSpace);
	ui.pInput->setText(QString::number(p));
	on_learningMethodComboBox_activated(learnMethod);
}

//! Activates/deactivates groupboxes depending on the state of \a
//! learningMethodComboBox.
void ClassificationConfigForm::on_learningMethodComboBox_activated(
	int whichOptions)
{
	switch (whichOptions)
	{
	case 0:
		ui.groupBox2->show();
		ui.groupBox3->hide();
		break;
	case 1:
		ui.groupBox3->show();
		ui.groupBox2->hide();
		break;
	}
}
} // cove

//@}
