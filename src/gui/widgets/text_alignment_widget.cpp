/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2013-2016 Kai Pastor
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


#include "text_alignment_widget.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QPushButton>
#include <QSignalMapper>

#include "../../object_text.h"
#include "../../tool_helpers.h"


TextObjectAlignmentDockWidget::TextObjectAlignmentDockWidget(TextObject* object, int horz_default, int vert_default, TextObjectEditorHelper* text_editor, QWidget* parent)
 : QDockWidget(tr("Alignment"), parent), object(object), text_editor(text_editor)
{
	setFocusPolicy(Qt::NoFocus);
	setFeatures(features() & ~QDockWidget::DockWidgetClosable);
	
	QWidget* widget = new QWidget();
	
	addHorzButton(0, QString::fromLatin1(":/images/text-align-left.png"), horz_default);
	addHorzButton(1, QString::fromLatin1(":/images/text-align-hcenter.png"), horz_default);
	addHorzButton(2, QString::fromLatin1(":/images/text-align-right.png"), horz_default);
	
	addVertButton(0, QString::fromLatin1(":/images/text-align-top.png"), vert_default);
	addVertButton(1, QString::fromLatin1(":/images/text-align-vcenter.png"), vert_default);
	addVertButton(2, QString::fromLatin1(":/images/text-align-baseline.png"), vert_default);
	addVertButton(3, QString::fromLatin1(":/images/text-align-bottom.png"), vert_default);
	
	QHBoxLayout* horz_layout = new QHBoxLayout();
	horz_layout->setMargin(0);
	horz_layout->setSpacing(0);
	horz_layout->addStretch(1);
	for (int i = 0; i < 3; ++i)
		horz_layout->addWidget(horz_buttons[i]);
	horz_layout->addStretch(1);
	
	QHBoxLayout* vert_layout = new QHBoxLayout();
	vert_layout->setMargin(0);
	vert_layout->setSpacing(0);
	for (int i = 0; i <4; ++i)
		vert_layout->addWidget(vert_buttons[i]);
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addLayout(horz_layout);
	layout->addLayout(vert_layout);
	widget->setLayout(layout);
	
	setWidget(widget);
	
	QSignalMapper* horz_mapper = new QSignalMapper(this);
	connect(horz_mapper, SIGNAL(mapped(int)), this, SLOT(horzClicked(int)));
	for (int i = 0; i < 3; ++i)
	{
		horz_mapper->setMapping(horz_buttons[i], i);
		connect(horz_buttons[i], SIGNAL(clicked()), horz_mapper, SLOT(map()));
	}
	
	QSignalMapper* vert_mapper = new QSignalMapper(this);
	connect(vert_mapper, SIGNAL(mapped(int)), this, SLOT(vertClicked(int)));
	for (int i = 0; i < 4; ++i)
	{
		vert_mapper->setMapping(vert_buttons[i], i);
		connect(vert_buttons[i], SIGNAL(clicked()), vert_mapper, SLOT(map()));
	}
}

bool TextObjectAlignmentDockWidget::event(QEvent* event)
{
	if (event->type() == QEvent::ShortcutOverride)
		event->accept();
	
	return QDockWidget::event(event);
}

void TextObjectAlignmentDockWidget::keyPressEvent(QKeyEvent* event)
{
	if (text_editor->keyPressEvent(event))
		event->accept();
	else
		QWidget::keyPressEvent(event);
}

void TextObjectAlignmentDockWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (text_editor->keyReleaseEvent(event))
		event->accept();
	else
		QWidget::keyReleaseEvent(event);
}

void TextObjectAlignmentDockWidget::addHorzButton(int index, const QString& icon_path, int horz_default)
{
	const TextObject::HorizontalAlignment horz_array[] = {TextObject::AlignLeft, TextObject::AlignHCenter, TextObject::AlignRight};
	const QString tooltips[] = {tr("Left"), tr("Center"), tr("Right")};
	
	horz_buttons[index] = new QPushButton(QIcon(icon_path), QString{});
	horz_buttons[index]->setFocusPolicy(Qt::NoFocus);
	horz_buttons[index]->setCheckable(true);
	horz_buttons[index]->setToolTip(tooltips[index]);
	if (horz_default == horz_array[index])
	{
		horz_buttons[index]->setChecked(true);
		horz_index = index;
	}	
}

void TextObjectAlignmentDockWidget::addVertButton(int index, const QString& icon_path, int vert_default)
{
	const TextObject::VerticalAlignment vert_array[] = {TextObject::AlignTop, TextObject::AlignVCenter, TextObject::AlignBaseline, TextObject::AlignBottom};
	const QString tooltips[] = {tr("Top"), tr("Center"), tr("Baseline"), tr("Bottom")};
	
	vert_buttons[index] = new QPushButton(QIcon(icon_path), QString{});
	vert_buttons[index]->setFocusPolicy(Qt::NoFocus);
	vert_buttons[index]->setCheckable(true);
	vert_buttons[index]->setToolTip(tooltips[index]);
	if (vert_default == vert_array[index])
	{
		vert_buttons[index]->setChecked(true);
		vert_index = index;
	}	
}

void TextObjectAlignmentDockWidget::horzClicked(int index)
{
	for (int i = 0; i < 3; ++i)
		horz_buttons[i]->setChecked(index == i);
	horz_index = index;
	emitAlignmentChanged();
}

void TextObjectAlignmentDockWidget::vertClicked(int index)
{
	for (int i = 0; i < 4; ++i)
		vert_buttons[i]->setChecked(index == i);
	vert_index = index;
	emitAlignmentChanged();
}

void TextObjectAlignmentDockWidget::emitAlignmentChanged()
{
	const TextObject::HorizontalAlignment horz_array[] = {TextObject::AlignLeft, TextObject::AlignHCenter, TextObject::AlignRight};
	const TextObject::VerticalAlignment vert_array[] = {TextObject::AlignTop, TextObject::AlignVCenter, TextObject::AlignBaseline, TextObject::AlignBottom};
	
	emit(alignmentChanged((int)horz_array[horz_index], (int)vert_array[vert_index]));
}
