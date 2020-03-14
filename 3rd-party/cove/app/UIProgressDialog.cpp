/*
 * Copyright (c) 2005-2019 Libor Pecháček.
 * Copyright 2020 Kai Pastor
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

#include "UIProgressDialog.h"

#include <QCoreApplication>

namespace cove {
//@{
//! \ingroup gui

/*! \class UIProgressDialog
 * \brief A proxy for a QProgressDialog
 * 
 * This class implements ProgressObserver and connects it to a modal
 * QProgressDialog. The dialog is closed when the proxy is destroyed
 * (or when the user clicks Cancel).
 */

/*! Constructor, essentially the same as QProgressDialog's constructor. Sets
 * progressbar range to 0-100.
 */
UIProgressDialog::UIProgressDialog(const QString& labelText,
                                   const QString& cancelButtonText,
                                   QWidget* creator, Qt::WindowFlags /*unused*/)
	: pDialog(labelText, cancelButtonText, 0, 100, creator)
{
	pDialog.setAutoClose(false);  // avoid reopening on late setValue().
	pDialog.setAutoReset(false);  // avoid jumping to zero when input is noisy.
	pDialog.setMinimumDuration(0);
	pDialog.setWindowModality(Qt::WindowModal); 
}

/*! Destructor.
 */
UIProgressDialog::~UIProgressDialog() = default;

/*! Implementation of ProgressObserver abstract method.
 */
void UIProgressDialog::setPercentage(int percentage)
{
	// QProgressDialog::setValue(int) will take care of calling
	// QApplication::processEvents() when the dialog is modal, but it
	// would make the dialog reappear if was already canceled and hidden.
	if (pDialog.wasCanceled())
		QCoreApplication::processEvents();
	else
		pDialog.setValue(percentage);
}

/*! Implementation of ProgressObserver abstract method, returns
 * value of the progress bar dialog cancel status.
 */
bool UIProgressDialog::isInterruptionRequested() const
{
	// Handle main thread events (such as UI input).
	QCoreApplication::processEvents();
	return pDialog.wasCanceled();
}

} // cove

//@}
