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

#include "colorseditform.h"
#include "ui_colorseditform.h"

#include <memory>

#include <QAbstractItemModel>
#include <QBrush>
#include <QColor>
#include <QColorDialog>
#include <QEvent>
#include <QFlags>
#include <QFrame>
#include <QItemDelegate>
#include <QModelIndex>
#include <QRadioButton>
#include <QStyleOptionViewItem>
#include <QTableView>

class QStyleOptionViewItem;
class QWidget;

namespace cove {
/*! \ingroup gui */
/*!@{ */

/*! \brief Specialization of QItemDelegate.

  Delegate spawning QColorDialog on the first two columns.
  */
class ColorsEditingDelegate : public QItemDelegate
{
	QRgb clr;

	//! Reimplemented QItemDelegate::createEditor returning 0 on the first two
	// columns, calling QItemDelegate::createEditor otherwise.
	QWidget* createEditor(QWidget* parent,
	                      const QStyleOptionViewItem& option,
	                      const QModelIndex& index) const override
	{
		if (index.column() <= 1)
		{
			return nullptr;
		}
		else
		{
			return QItemDelegate::createEditor(parent, option, index);
		}
	}

	//! Reimplemented QItemDelegate::editorEvent spawning QColorDialog and
	// returning true on the first two columns, calling
	// QItemDelegate::editorEvent otherwise.
	bool editorEvent(QEvent* event, QAbstractItemModel* model,
	                 const QStyleOptionViewItem& option,
	                 const QModelIndex& index) override
	{
		if (event->type() == QEvent::MouseButtonDblClick && index.column() <= 1)
		{
			QVariant v = model->data(model->index(index.row(), 0, index),
									 Qt::BackgroundRole);
			QColor c = v.value<QBrush>().color();
			clr = QColorDialog::getRgba(c.rgb());
			setModelData(nullptr, model, index);
			event->accept();
			return true;
		}
		return QItemDelegate::editorEvent(event, model, option, index);
	}

	//! Reimplemented QItemDelegate::setModelData forwarding data from the form
	//! to
	// the model.
	void setModelData(QWidget* editor, QAbstractItemModel* model,
	                  const QModelIndex& index) const override
	{
		if (index.column() <= 1)
		{
			model->setData(index, QColor(clr));
		}
		else
		{
			QItemDelegate::setModelData(editor, model, index);
		}
	}
};

/*! \class ColorsListModel
 * \brief A class implementing QAbstractTableModel.
 *
 * It is a table with color field in
 * the first column, RGB value of the color in the second and a textual comment
 * in the third. * */
/*! \var ColorsListModel::colors Colors that should be displayed. */
/*! \var ColorsListModel::comments Comments for those colors. */

//! Default constructor, marked private.
ColorsListModel::ColorsListModel() = default;

//! Constructor creating dialog with colors and comments.
ColorsListModel::ColorsListModel(const std::vector<QRgb>& colors,
								 const std::vector<QString>& comments)
	: colors(colors)
	, comments(comments)
{
}

//! Reimplementation of QTableModel member.
int ColorsListModel::rowCount(const QModelIndex& /*parent*/) const
{
	return colors.size();
}

//! Reimplementation of QTableModel member.
int ColorsListModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 3;
}

//! Reimplementation of QTableModel member.
QVariant ColorsListModel::data(const QModelIndex& index, int role) const
{
	if (index.column() == 0 && role == Qt::BackgroundRole)
	{
		return QVariant(QBrush(QColor(colors[index.row()])));
	}
	else if (index.column() == 1 && role == Qt::DisplayRole)
	{
		return QVariant(QString("[%1,%2,%3]")
							.arg(qRed(colors[index.row()]))
							.arg(qGreen(colors[index.row()]))
							.arg(qBlue(colors[index.row()])));
	}
	else if (index.column() == 2 && role == Qt::DisplayRole &&
			 index.row() < int(comments.size()))
	{
		return QVariant(comments[index.row()]);
	}
	else
	{
		return QVariant();
	}
}

//! Reimplementation of QTableModel member.
QVariant ColorsListModel::headerData(int section, Qt::Orientation orientation,
									 int role) const
{
	QVariant rv = QVariant();

	if (role == Qt::DisplayRole)
	{
		if (orientation == Qt::Horizontal)
		{
			switch (section)
			{
			case 0:
				rv = tr("Color");
				break;
			case 1:
				rv = tr("RGB");
				break;
			case 2:
				rv = tr("Comment");
				break;
			}
		}
		else if (orientation == Qt::Vertical)
		{
			rv = section + 1;
		}
	}
	return rv;
}

Qt::ItemFlags ColorsListModel::flags(const QModelIndex& index) const
{
	if (!index.isValid()) return Qt::ItemIsEnabled;

	return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

//! Reimplementation of QTableModel member.
bool ColorsListModel::setData(const QModelIndex& index, const QVariant& value,
							  int role)
{
	if (index.isValid() && role == Qt::EditRole)
	{
		if (index.column() == 2)
		{
			comments[index.row()] = value.toString();
		}
		else
		{
			QColor color = value.value<QColor>();
			colors[index.row()] = color.rgb();
			emit dataChanged(index, index);
		}
		return true;
	}
	return false;
}

auto ColorsListModel::getColors()
{
	return colors;
}

auto ColorsListModel::getComments()
{
	return comments;
}

/*! \class ColorsEditForm
  \brief Form used when configuring initial colors. */

/*! \enum ColorsEditForm::ColorsSource
  \brief Enum for different color source options. */

/*! \var ColorsEditForm::ColorsSource ColorsEditForm::Random
 Initial colors are completely random. */
/*! \var ColorsEditForm::ColorsSource ColorsEditForm::RandomFromImage
 Initial colors are taken from image. */
/*! \var ColorsEditForm::ColorsSource ColorsEditForm::Predefined
 Initial colors are taken from the table in this form. */

//! Form constructor, adds QitemDelegate to initialColorsListView.
ColorsEditForm::ColorsEditForm(QWidget* parent)
	: QDialog(parent)
	, m(nullptr)
	, d(nullptr)
{
	ui.setupUi(this);
	ColorsEditingDelegate* d = new ColorsEditingDelegate();
	ui.initialColorsListView->setItemDelegate(d);
}

//! Destructor.
ColorsEditForm::~ColorsEditForm()
{
	if (m)
	{
		ui.initialColorsListView->setModel(nullptr);
		delete m;
	}
	if (d)
	{
		ui.initialColorsListView->setItemDelegate(nullptr);
		delete d;
	}
}

std::vector<QRgb> ColorsEditForm::getColors()
{
	return m ? m->getColors() : std::vector<QRgb>();
}

std::vector<QString> ColorsEditForm::getComments()
{
	return m ? m->getComments() : std::vector<QString>();
}

//! Creates model from color data and puts it into the initialColorsListView.
void ColorsEditForm::setColors(const std::vector<QRgb>& colors,
							   const std::vector<QString>& comments)
{
	if (m)
	{
		ui.initialColorsListView->setModel(nullptr);
		delete m;
		m = nullptr;
	}

	if (!colors.empty())
	{
		m = new ColorsListModel(colors, comments);
		ui.initialColorsListView->setModel(m);
	}
}

//! Returns selected colors source.
ColorsEditForm::ColorsSource ColorsEditForm::getColorsSource()
{
	if (ui.randomColorsButton->isChecked()) return Random;
	if (ui.randomFromImageButton->isChecked()) return RandomFromImage;
	if (ui.predefinedColorsButton->isChecked()) return Predefined;
	return Random;
}

//! Sets selected colors source.
void ColorsEditForm::setColorsSource(ColorsSource s)
{
	ui.randomColorsButton->setChecked(s == Random);
	ui.randomFromImageButton->setChecked(s == RandomFromImage);
	ui.predefinedColorsButton->setChecked(s == Predefined);
	on_predefinedColorsButton_toggled(s == Predefined);
}

//! Enables/disables the initColorsFrame depending on the selected color source.
void ColorsEditForm::on_predefinedColorsButton_toggled(bool checked)
{
	ui.initColorsFrame->setEnabled(checked);
}
} // cove

//@}
