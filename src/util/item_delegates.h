/*
 *    Copyright 2013 Kai Pastor
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

#ifndef _OPENORIENTEERING_ITEM_DELEGATES_H_
#define _OPENORIENTEERING_ITEM_DELEGATES_H_

#include <QItemDelegate>
#include <QStyledItemDelegate>


/**
 * An item delegate which respects colors from the model even when the item is selected.
 * 
 * ColorItemDelegate uses the background color and/or foreground color from the
 * model data with Qt::BackgroundRole and/or Qt::ForegroundRole, respectively.
 * However, an extra frame is drawn in the style's QPalette::Highlight color.
 */
class ColorItemDelegate : public QStyledItemDelegate
{
Q_OBJECT

public:
	/** Constructs a new ColorItemDelegate. */
	ColorItemDelegate(QObject* parent = NULL);
	
	/** Renders the delegate. Reimplemented from QStyledItemDelegate. */
	virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};



/**
 *  A QItemDelegate which provides an editor of type (integer) spin box.
 * 
 *  Unlike the default editor behaviour, the spin box will commit each single
 *  change as soon as control returns to the event loop.
 * 
 *  This delegate provides an additional method 
 *    setModelData(QAbstractItemModel*, const QModelIndex&, int value)
 *  which allows to initialize the model for a given value.
 */
class SpinBoxDelegate : public QItemDelegate
{
Q_OBJECT
public:
	/** Creates a new SpinBoxDelegate.
	 *  @param parent The parent object which will be passed to QItemDelegate.
	 *  @param min    The minimum valid value.
	 *  @param max    The maximum valid value.
	 *  @param unit   The unit of measurement which will be displayed as 
	 *                suffix to the value.
	 *  @param step   The size of single step when using the spin box button.
	 */
	SpinBoxDelegate(QObject* parent, int min, int max, const QString &unit = "", int step = 0);
	
	/** Creates a new SpinBoxDelegate.
	 *  @param min    The minimum valid value.
	 *  @param max    The maximum valid value.
	 *  @param unit   The unit of measurement which will be displayed as 
	 *                suffix to the value.
	 *  @param step   The size of single step when using the spinbox button.
	 */
	SpinBoxDelegate(int min, int max, const QString &unit = "", int step = 0);
	
	/** Returns a new QSpinBox configured according to the delegates properties. 
	 *  @see QItemDelegate::createEditor().
	 */
	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	
	/** Updates the spin box value from the model data with role Qt::UserData.
	 *  @see QItemDelegate::setEditorData().
	 */
	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
	
	/** Updates the model from the spin box value. The integer value is stored
	 *  with role Qt::UserData. Qt::DisplayValue is set to the number followed
	 *  by a space character and the unit.
	 *  @see QItemDelegate::setModelData().
	 */
	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
	
	/** Updates the model from the given value. The integer value is stored
	 *  with role Qt::UserData. Qt::DisplayValue is set to the number followed
	 *  by a space character and the unit.
	 * 
	 *  This method can be used to initialize the model for a given value.
	 */
	virtual void setModelData(QAbstractItemModel* model, const QModelIndex& index, int value) const;
	
	/** @see QItemDelegate::updateEditorGeometry().
	 */
	virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	
private:
	const int min;
	const int max;
	const int step;
	const QString unit;
};

#endif