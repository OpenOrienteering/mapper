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


#include "item_delegates.h"

#if QT_VERSION < 0x050000
#include <QtGui>
#else
#include <QtWidgets>
#endif

#include "../util_gui.h"


// ### ColorItemDelegate ###

ColorItemDelegate::ColorItemDelegate(QObject* parent)
 : QStyledItemDelegate(parent)
{
	// Nothing.
}

void ColorItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!option.state.testFlag(QStyle::State_Selected))
	{
		// Use the style options as is.
		QStyledItemDelegate::paint(painter, option, index);
	}
	else
	{
		// Use the model's background and/or foreground.
		QStyleOptionViewItem background_option(option);
		QPalette& palette(background_option.palette);
		
		QVariant background_var(index.data(Qt::BackgroundRole));
		if (!background_var.isNull())
		{
			palette.setColor(QPalette::Highlight, background_var.value<QColor>());
		}
		QVariant foreground_var(index.data(Qt::ForegroundRole));
		if (!foreground_var.isNull())
		{
			palette.setColor(QPalette::HighlightedText, foreground_var.value<QColor>());
		}
		QStyledItemDelegate::paint(painter, background_option, index);
		
		// Draw an extra frame.
		painter->save();
		painter->setPen(QPen(option.palette.color(QPalette::Highlight), 1));
		painter->drawRect(option.rect.adjusted(1, 1, -2, -2));
		painter->restore();
	}
}



//### SpinBoxDelegate ###

SpinBoxDelegate::SpinBoxDelegate(QObject* parent, int min, int max, const QString& unit, int step)
 : QItemDelegate(parent),
   min(min),
   max(max),
   step(step),
   unit(unit)
{
	// nothing
}

SpinBoxDelegate::SpinBoxDelegate(int min, int max, const QString& unit, int step)
 : QItemDelegate(),
   min(min),
   max(max),
   step(step),
   unit(unit)
{
	// nothing
}

QWidget* SpinBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	
	QSpinBox* spinbox = Util::SpinBox::create(min, max, unit, step);
	spinbox->setParent(parent);
	
	// Commit each change immediately when returning to event loop
	QSignalMapper* signal_mapper = new QSignalMapper(spinbox);
	signal_mapper->setMapping(spinbox, spinbox);
	connect(spinbox, SIGNAL(valueChanged(int)), signal_mapper, SLOT(map()), Qt::QueuedConnection);
	connect(signal_mapper, SIGNAL(mapped(QWidget*)), this, SIGNAL(commitData(QWidget*)));
	
	return spinbox;
}

void SpinBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	int value = index.model()->data(index, Qt::UserRole).toInt();
	static_cast< QSpinBox* >(editor)->setValue(value);
}

void SpinBoxDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QSpinBox* spinBox = static_cast< QSpinBox* >(editor);
	spinBox->interpretText();
	setModelData(model, index, spinBox->value());
}

void SpinBoxDelegate::setModelData(QAbstractItemModel* model, const QModelIndex& index, int value) const
{
	QMap< int, QVariant > data(model->itemData(index));
	data[Qt::UserRole] = value;
	data[Qt::DisplayRole] = unit.isEmpty() ?
	  QLocale().toString(value) : 
	  (QLocale().toString(value) % " " % unit);
	model->setItemData(index, data);
}

void SpinBoxDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}



//### PercentageDelegate ###

PercentageDelegate::PercentageDelegate(QObject* parent, int step)
 : QStyledItemDelegate(parent),
   step(step)
{
	unit = QLatin1String(" ") % tr("%");
}

PercentageDelegate::PercentageDelegate(int step)
 : QStyledItemDelegate(),
   step(step)
{
	unit = QLatin1String(" ") % tr("%");
}

QString PercentageDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
	int int_value = qRound(value.toFloat() * 100);
	return locale.toString(int_value) % unit;
}

QWidget* PercentageDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);
	
	QSpinBox* spinbox = Util::SpinBox::create(0, 100, unit, step);
	spinbox->setParent(parent);
	
	// Commit each change immediately when returning to event loop
	QSignalMapper* signal_mapper = new QSignalMapper(spinbox);
	signal_mapper->setMapping(spinbox, spinbox);
	connect(spinbox, SIGNAL(valueChanged(int)), signal_mapper, SLOT(map()), Qt::QueuedConnection);
	connect(signal_mapper, SIGNAL(mapped(QWidget*)), this, SIGNAL(commitData(QWidget*)));
	
	return spinbox;
}

void PercentageDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	int value = qRound(index.model()->data(index, Qt::DisplayRole).toFloat() * 100);
	static_cast< QSpinBox* >(editor)->setValue(value);
}

void PercentageDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
	QSpinBox* spinBox = static_cast< QSpinBox* >(editor);
	spinBox->interpretText();
	QMap< int, QVariant > data(model->itemData(index));
	data[Qt::DisplayRole] = (float)spinBox->value() / 100.0;
	model->setItemData(index, data);
}

void PercentageDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
