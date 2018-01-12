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

#include "core/objects/text_object.h"
#include "tools/text_object_editor_helper.h"


namespace OpenOrienteering {

namespace {

struct AlignmentOption
{
	int alignment;
	const char* text;
	const char* icon;
};

const AlignmentOption horizontal_options[] = { 
    { TextObject::AlignLeft,    QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Left"),    ":/images/text-align-left.png" },
    { TextObject::AlignHCenter, QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Center"),  ":/images/text-align-hcenter.png" },
    { TextObject::AlignRight,   QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Right"),   ":/images/text-align-right.png" },
};

const AlignmentOption vertical_options[] = { 
    { TextObject::AlignTop,     QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Top"),     ":/images/text-align-top.png" },
    { TextObject::AlignVCenter, QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Center"),  ":/images/text-align-vcenter.png" },
    { TextObject::AlignBaseline,QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Baseline"),":/images/text-align-baseline.png" },
    { TextObject::AlignBottom,  QT_TRANSLATE_NOOP("OpenOrienteering::TextObjectAlignmentDockWidget", "Bottom"),  ":/images/text-align-bottom.png" },
};


}  // namespace


TextObjectAlignmentDockWidget::TextObjectAlignmentDockWidget(TextObjectEditorHelper* text_editor, QWidget* parent)
: QDockWidget { tr("Alignment"), parent }
, text_editor { text_editor }
, horizontal_alignment { text_editor->object()->getHorizontalAlignment() }
, vertical_alignment   { text_editor->object()->getVerticalAlignment() }
{
	setFocusPolicy(Qt::NoFocus);
	setFeatures(features() & ~QDockWidget::DockWidgetClosable);
	
	QHBoxLayout* horizontal_layout = new QHBoxLayout();
	horizontal_layout->setMargin(0);
	horizontal_layout->setSpacing(0);
	horizontal_layout->addStretch(1);
	for (size_t i = 0; i < 3; ++i)
	{
		auto& entry = horizontal_options[i];
		auto button = makeButton(QString::fromLatin1(entry.icon), tr(entry.text));
		if (entry.alignment == horizontal_alignment)
			button->setChecked(true);
		horizontal_layout->addWidget(button);
		connect(button, &QPushButton::clicked, this, &TextObjectAlignmentDockWidget::horizontalClicked);
		horizontal_buttons[i] = button;
	}
	horizontal_layout->addStretch(1);
	
	QHBoxLayout* vertical_layout = new QHBoxLayout();
	vertical_layout->setMargin(0);
	vertical_layout->setSpacing(0);
	for (size_t i = 0; i < 4; ++i)
	{
		auto& entry = vertical_options[i];
		auto button = makeButton(QString::fromLatin1(entry.icon), tr(entry.text));
		if (entry.alignment == vertical_alignment)
			button->setChecked(true);
		vertical_layout->addWidget(button);
		connect(button, &QPushButton::clicked, this, &TextObjectAlignmentDockWidget::verticalClicked);
		vertical_buttons[i] = button;
	}
	
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addLayout(horizontal_layout);
	layout->addLayout(vertical_layout);
	
	QWidget* widget = new QWidget();
	widget->setLayout(layout);
	setWidget(widget);
}

TextObjectAlignmentDockWidget::~TextObjectAlignmentDockWidget()
{
	// nothing, not inlined
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

void TextObjectAlignmentDockWidget::horizontalClicked()
{
	auto alignment = horizontal_alignment;
	auto const button = sender();
	for (size_t i = 0; i < 3; ++i)
	{
		if (button == horizontal_buttons[i])
			alignment = horizontal_options[i].alignment;
		else
			horizontal_buttons[i]->setChecked(false);
	}
	if (alignment != horizontal_alignment)
	{
		horizontal_alignment = alignment;
		emit alignmentChanged(horizontal_alignment, vertical_alignment);
	}
}

void TextObjectAlignmentDockWidget::verticalClicked()
{
	auto alignment = vertical_alignment;
	auto const button = sender();
	for (size_t i = 0; i < 4; ++i)
	{
		if (button == vertical_buttons[i])
			alignment = vertical_options[i].alignment;
		else
			vertical_buttons[i]->setChecked(false);
	}
	if (alignment != vertical_alignment)
	{
		vertical_alignment = alignment;
		emit alignmentChanged(horizontal_alignment, vertical_alignment);
	}
}

QPushButton* TextObjectAlignmentDockWidget::makeButton(const QString& icon_path, const QString& text) const
{
	auto button = new QPushButton(QIcon(icon_path), {});
	button->setToolTip(text);
	button->setFocusPolicy(Qt::NoFocus);
	button->setCheckable(true);
	return button;
}


}  // namespace OpenOrienteering
