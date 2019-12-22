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

#ifndef COVE_COLORSEDITFORM_H
#define COVE_COLORSEDITFORM_H

#include <vector>

#include <QAbstractTableModel>
#include <QDialog>
#include <QObject>
#include <QRgb>
#include <QString>
#include <QVariant>
#include <Qt>

#include "ui_colorseditform.h"

class QModelIndex;
class QWidget;

namespace cove {
class ColorsListModel : public QAbstractTableModel
{
	Q_OBJECT

private:
	std::vector<QRgb> colors;
	std::vector<QString> comments;
	ColorsListModel();

public:
	ColorsListModel(const std::vector<QRgb>& colors,
	                const std::vector<QString>& comments);
	virtual int rowCount(const QModelIndex&) const;
	virtual int columnCount(const QModelIndex&) const;
	virtual QVariant data(const QModelIndex& index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation,
	                            int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value,
	                     int role);

	auto getColors();
	auto getComments();
};

class ColorsEditingDelegate;

class ColorsEditForm : public QDialog
{
	Q_OBJECT

private:
	Ui::ColorsEditForm ui;
	ColorsListModel* m;
	ColorsEditingDelegate* d;

public:
	enum ColorsSource
	{
		Random = 0,
		RandomFromImage,
		Predefined
	};
	ColorsEditForm(QWidget* parent = nullptr);
	~ColorsEditForm();
	std::vector<QRgb> getColors();
	std::vector<QString> getComments();
	void setColors(const std::vector<QRgb>& colors,
	               const std::vector<QString>& comments);
	ColorsSource getColorsSource();
	void setColorsSource(ColorsSource s);

public slots:
	void on_predefinedColorsButton_toggled(bool checked);  // clazy:exclude=connect-by-name
};
} // cove

#endif
