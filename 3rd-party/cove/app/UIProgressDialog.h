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

#ifndef COVE_PROGRESSDIALOG_H
#define COVE_PROGRESSDIALOG_H

#include <Qt>
#include <QProgressDialog>
#include <QString>

#include "libvectorizer/ProgressObserver.h"

class QWidget;

namespace cove {

class UIProgressDialog : public ProgressObserver
{
	QProgressDialog pDialog;
	
public:
	UIProgressDialog(const QString& labelText, const QString& cancelButtonText,
	                 QWidget* creator = nullptr, Qt::WindowFlags f = {});
	UIProgressDialog(const UIProgressDialog&) = delete;
	UIProgressDialog(UIProgressDialog&&) = delete;
	UIProgressDialog operator=(const UIProgressDialog&) = delete;
	UIProgressDialog operator=(UIProgressDialog&&) = delete;
	~UIProgressDialog() override;
	void setPercentage(int percentage) override;
	bool isInterruptionRequested() const override;
};

} // cove

#endif
