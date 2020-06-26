/*
 *    Copyright 2020 Kai Pastor
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

#ifndef OPENORIENTEERING_PAINT_ON_TEMPLATE_FEATURE_H
#define OPENORIENTEERING_PAINT_ON_TEMPLATE_FEATURE_H

#include <QtGlobal>
#include <QObject>
#include <QString>

class QAction;

namespace OpenOrienteering {

class MapEditorController;
class Template;


/**
 * Provides an interactive feature for painting on templates (aka scribbling).
 * 
 * \todo Move translations to this context.
 */
class PaintOnTemplateFeature : public QObject
{
	Q_OBJECT
	
public:
	~PaintOnTemplateFeature() override;
	
	PaintOnTemplateFeature(MapEditorController& controller);
	
	/**
	 * Changes the state of the provided actions.
	 */
	void setEnabled(bool enabled);
	
	/**
	 * The action which tries to immediately activate the painting tool.
	 */
	QAction* paintAction() { return paint_action; }
	
	/**
	 * The action which lets the use choose a template, or create a new template.
	 */
	QAction* selectAction() { return select_action; }
	
protected:
	/**
	 * Resets internal state which relies on a particular template.
	 */
	void templateAboutToBeDeleted(int pos, Template* temp);
	
	/**
	 * Reacts on triggering the paint action.
	 */
	void paintClicked(bool checked);
	
	/**
	 * Reacts on triggering the paint action.
	 */
	void selectTemplateClicked();
	
	/**
	 * Returns a template selected by the user.
	 * 
	 * May return nullptr when cancelled, or on error.
	 */
	Template* selectTemplate() const;
	
	/**
	 * Activates the painting tool for the given template.
	 */
	void startPainting(Template* temp);
	
	/**
	 * Terminates the painting tool.
	 */
	void finishPainting();
	
private:
	MapEditorController& controller;
	QAction* paint_action = nullptr;      // child of this
	QAction* select_action = nullptr;     // child of this
	Template* last_template = nullptr;
	
	Q_DISABLE_COPY(PaintOnTemplateFeature)
};


}  // namespace OpenOrienteering

#endif
