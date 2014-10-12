/*
 *    Copyright 2014 Kai Pastor
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

#include "autosave_dialog.h"

#include "main_window.h"

AutosaveDialog::AutosaveDialog(QString path, QString autosave_path, QString actual_path, MainWindow* parent, Qt::WindowFlags f)
: QDialog(parent, f)
, main_window(parent)
, original_path(path)
, autosave_path(autosave_path)
, resolved(false)
{
	const QString text_template = QString("<b>%1</b><br/>%2<br>%3");
	
	QFileInfo autosaved_file_info(autosave_path);
	autosaved_text.setHtml(text_template.
	   arg(tr("Autosaved file")).
	   arg(autosaved_file_info.lastModified().toLocalTime().toString()).
	   arg(tr("%n bytes", 0, autosaved_file_info.size())));
	
	QFileInfo user_saved_file_info(path);
	user_saved_text.setHtml(text_template.
	   arg(tr("File saved by the user")).
	   arg(user_saved_file_info.lastModified().toLocalTime().toString()).
	   arg(tr("%n bytes", 0, user_saved_file_info.size())));
	
	layout = new QVBoxLayout();
	setLayout(layout);
	
	setWindowTitle(tr("File recovery"));
	
	QString intro_text = tr("File %1 was not properly closed. At the moment, there are two versions:");
	QLabel* label = new QLabel(intro_text.arg(QString("<b>%1</b>").arg(user_saved_file_info.fileName())));
	label->setWordWrap(true);
	layout->addWidget(label);
	
	list_widget = new QListWidget();
	list_widget->setItemDelegate(new TextDocItemDelegate(this, this));
	list_widget->setSelectionMode(QAbstractItemView::SingleSelection);
	QListWidgetItem* item = new QListWidgetItem(list_widget, QListWidgetItem::UserType);
	item->setData(Qt::UserRole, QVariant(1));
	item = new QListWidgetItem(list_widget, QListWidgetItem::UserType);
	item->setData(Qt::UserRole, QVariant(2));
	layout->addWidget(list_widget);
	
	label = new QLabel(tr("Save the active file to remove the conflicting version."));
	label->setWordWrap(true);
	layout->addWidget(label);
	
	setSelectedPath(actual_path);
	
	connect(list_widget, SIGNAL(currentRowChanged(int)), this, SLOT(currentRowChanged(int)), Qt::QueuedConnection);
}

AutosaveDialog::~AutosaveDialog()
{
	// Nothing
}

// slot
int AutosaveDialog::exec()
{
	QDialogButtonBox button_box(QDialogButtonBox::Open | QDialogButtonBox::Cancel);
	layout->addWidget(&button_box);
	connect(&button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(&button_box, SIGNAL(rejected()), this, SLOT(reject()));
	const int result = QDialog::exec();
	resolved = true;
	return result;
}

// slot
void AutosaveDialog::autosaveConflictResolved()
{
	resolved = true;
	if (!isModal())
		close();
}

void AutosaveDialog::closeEvent(QCloseEvent *event)
{
	if (main_window && !resolved)
		event->setAccepted(main_window->closeFile());
	else
		event->setAccepted(resolved);
}

QString AutosaveDialog::selectedPath() const
{
	const int row = list_widget->currentRow();
	switch (row)
	{
	case 0:
		return autosave_path;
		break;
	case 1:
		return original_path;
		break;
	case -1:
		break; // Nothing selected?
	default:
		Q_ASSERT(false && "Undefined index");
	}
	return QString();
}

// slot
void AutosaveDialog::setSelectedPath(QString path)
{
	if (path == original_path)
		list_widget->setCurrentRow(1);
	else if (path == autosave_path)
		list_widget->setCurrentRow(0);
	else
		list_widget->setCurrentRow(-1);
}

void AutosaveDialog::currentRowChanged(int row)
{
	switch (row)
	{
	case 0:
		emit pathSelected(autosave_path);
		break;
	case 1:
		emit pathSelected(original_path);
		break;
	case -1:
		return; // Nothing selected?
	default:
		Q_ASSERT(false && "Undefined index");
	}
}

const QTextDocument* AutosaveDialog::textDoc(const QModelIndex& index) const
{
	const QTextDocument* ret = NULL;
	
	bool ok = true;
	int i = index.data(Qt::UserRole).toInt(&ok);
	if (ok)
	{
		switch (i)
		{
		case 1:
			ret = &autosaved_text;
			break;
		case 2:
			ret = &user_saved_text;
			break;
		default:
			Q_ASSERT(false && "Undefined index");
		}
	}
	else
	{
		Q_ASSERT(false && "Invalid data for UserRole");
	}
	
	return ret;
}
