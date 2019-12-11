/*
 *    Copyright 2016 Mitchell Krome
 *    Copyright 2017-2019 Kai Pastor
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


#ifndef OPENORIENTEERING_TAG_SELECT_WIDGET_H
#define OPENORIENTEERING_TAG_SELECT_WIDGET_H

#include <QtGlobal>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QTableWidget>

class QShowEvent;
class QToolButton;
class QWidget;

namespace OpenOrienteering {

class ObjectQuery;


/**
 * This widget allows the user to make selections based on an objects tags.
 */
class TagSelectWidget : public QTableWidget
{
Q_OBJECT
public:
	TagSelectWidget(QWidget* parent = nullptr);
	~TagSelectWidget() override;
	
	QWidget* makeButtons(QWidget* parent = nullptr);
	
	/**
	 * Builds a query based on the current state of the query table.
	 * 
	 * Returns an invalid query on error.
	 */
	ObjectQuery makeQuery() const;
	
protected:
	void showEvent(QShowEvent* event) override;
	
private:
	/**
	 * Returns a new QToolButton with a unified appearance.
	 */
	QToolButton* newToolButton(const QIcon& icon, const QString& text);
	
	void addRow();
	void deleteRow();
	void moveRow(bool up);
	void moveRowDown();
	void moveRowUp();

	void addRowItems(int row);
	void onCellChanged(int row, int column);
	void onCurrentCellChanged(int current_row, int current_column, int previous_row, int previous_column);
	
	QToolButton* delete_button;
	QToolButton* move_down_button;
	QToolButton* move_up_button;

	Q_DISABLE_COPY(TagSelectWidget)
};


}  // namespace OpenOrienteering

#endif
