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

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>

#include "app/UIProgressDialog.h"

namespace cove {
//@{
//! \ingroup gui

/*! \class UIProgressDialog
 * \brief QProgressDialog that implements ProgressObserver and is always
 * modal. Instantiate UIProgressDialog and pass it as ProgressObserver to
 * Vectorizer.
 */

/*! Constructor, essentially the same as QProgressDialog's constructor. Sets
 * progressbar range to 0-100.
 */
UIProgressDialog::UIProgressDialog(const QString& labelText,
								   const QString& cancelButtonText,
								   QWidget* creator, Qt::WindowFlags)
	: QObject(creator)
	, canceled(false)
	, pDialog(labelText, cancelButtonText, 0, 100, creator)
{
	pDialog.setMinimumDuration(0);
	pDialog.setWindowModality(Qt::WindowModal);
	connect(this, SIGNAL(percentageUpdated(int)), &pDialog,
			SLOT(setValue(int)));
}

/*! Destructor. Sets progress bar dialog value to 100 to make it disappear.
 */
UIProgressDialog::~UIProgressDialog()
{
	pDialog.setValue(100);
}

/*! Implementation of ProgressObserver abstract method, emits progressbar
 * update signals in a thread safe manner.
 */
void UIProgressDialog::percentageChanged(int percentage)
{
	// according to documentation QProgressDialog::setValue(int) will take
	// care of calling QApplication::processEvents() when the dialog is modal
	emit percentageUpdated(percentage);
}

/*! Implementation of ProgressObserver abstract method, returns
 * value of the progress bar dialog cancel status.
 */
bool UIProgressDialog::getCancelPressed()
{
	return pDialog.wasCanceled();
}
} // cove

//@}
