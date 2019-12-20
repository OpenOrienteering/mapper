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

#include <climits>
#include <limits>

#include <QDoubleValidator>
#include <QIntValidator>

#include "app/vectorizationconfigform.h"
#include "ui_vectorizationconfigform.h"

namespace cove {
VectorizationConfigForm::VectorizationConfigForm(QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	QDoubleValidator* dv;
	ui.shortPathLineEdit->setValidator(new QIntValidator(2, INT_MAX, this));
	ui.joinDistanceLineEdit->setValidator(
		dv = new QDoubleValidator(1, std::numeric_limits<double>::infinity(), 1,
								  this));
	ui.distDirBalanceLineEdit->setValidator(
		new QDoubleValidator(0, 1, 2, this));
}

void VectorizationConfigForm::accept()
{
	on_simpleConnectionsCheckBox_toggled(simpleConnectionsOnly);
	on_doConnectionsCheckBox_toggled(doConnections);
	speckleSize = ui.shortPathLineEdit->text().toDouble();
	doConnections = ui.doConnectionsCheckBox->checkState() == Qt::Checked;
	joinDistance = ui.joinDistanceLineEdit->text().toDouble();
	simpleConnectionsOnly =
		ui.simpleConnectionsCheckBox->checkState() == Qt::Checked;
	distDirBalance = ui.distDirBalanceLineEdit->text().toDouble();
	QDialog::accept();
}

int VectorizationConfigForm::exec()
{
	on_simpleConnectionsCheckBox_toggled(simpleConnectionsOnly);
	on_doConnectionsCheckBox_toggled(doConnections);
	ui.shortPathLineEdit->setText(QString::number(speckleSize));
	ui.doConnectionsCheckBox->setCheckState(doConnections ? Qt::Checked
														  : Qt::Unchecked);
	ui.joinDistanceLineEdit->setText(QString::number(joinDistance));
	ui.simpleConnectionsCheckBox->setCheckState(
		simpleConnectionsOnly ? Qt::Checked : Qt::Unchecked);
	ui.distDirBalanceLineEdit->setText(QString::number(distDirBalance));

	return QDialog::exec();
}

void VectorizationConfigForm::on_doConnectionsCheckBox_toggled(bool state)
{
	ui.joinDistanceLineEdit->setEnabled(state);
	ui.simpleConnectionsCheckBox->setEnabled(state);

	if (state)
		ui.distDirBalanceLineEdit->setEnabled(
			ui.simpleConnectionsCheckBox->checkState() != Qt::Checked);
	else
		ui.distDirBalanceLineEdit->setEnabled(false);
}

void VectorizationConfigForm::on_simpleConnectionsCheckBox_toggled(bool state)
{
	ui.distDirBalanceLineEdit->setEnabled(!state);
}
} // cove
