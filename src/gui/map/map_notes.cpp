/*
 *    Copyright 2012 Thomas Schöps
 *    Copyright 2024 Matthias Kühlewein
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


#include "map_notes.h"

#include <Qt>
#include <QtGlobal>
#include <QDialogButtonBox>
#include <QFlags>
#include <QFontMetrics>
#include <QLatin1Char>
#include <QRect>
#include <QSize>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "core/map.h"


namespace OpenOrienteering {

MapNotesDialog::MapNotesDialog(QWidget* parent, Map& map)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
, map { map }
{
	setWindowTitle(tr("Map notes"));
	
	text_edit = new QTextEdit();
	text_edit->setPlainText(map.getMapNotes());
	auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	auto* layout = new QVBoxLayout();
	layout->addWidget(text_edit);
	layout->addWidget(button_box);
	setLayout(layout);
	
	const auto max_size = parent ? parent->window()->size() : QSize{800, 600};
	const QFontMetrics text_font_metrics(text_edit->currentFont());
	auto preferred_width = text_font_metrics.boundingRect(QString(60, QLatin1Char('m'))).width();
	const auto bounding_rect = text_font_metrics.boundingRect(0, 0, preferred_width, max_size.height(), Qt::TextWordWrap, map.getMapNotes());
	auto width = qBound(300, bounding_rect.width() + 60, max_size.width());
	auto height = qBound(200, bounding_rect.height() + 80, max_size.height());
	resize(width, height);
	
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(button_box, &QDialogButtonBox::accepted, this, &MapNotesDialog::okClicked);	
}

MapNotesDialog::~MapNotesDialog() = default;

// slot
void MapNotesDialog::okClicked()
{
	auto text = text_edit->toPlainText();
	if (text != map.getMapNotes())
	{
		map.setMapNotes(text);
		map.setHasUnsavedChanges(true);
	}
	
	accept();
}

}  // namespace OpenOrienteering
