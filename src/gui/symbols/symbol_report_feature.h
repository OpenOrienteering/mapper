/*
 *    Copyright 2024 Kai Pastor
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

#ifndef OPENORIENTEERING_SYMBOL_REPORT_FEATURE_H
#define OPENORIENTEERING_SYMBOL_REPORT_FEATURE_H

#include <QtGlobal>
#include <QObject>
// IWYU pragma: no_include <QString>

class QAction;

namespace OpenOrienteering {

class MapEditorController;

/**
 * Provides a UI feature for generating symbol set reports.
 */
class SymbolReportFeature : public QObject
{
	Q_OBJECT
	
public:
	SymbolReportFeature(MapEditorController& controller);
	
	~SymbolReportFeature() override;
	
	void setEnabled(bool enabled);
	
	QAction* showDialogAction() { return show_action; }
	
private:
	void showDialog();
	
	MapEditorController& controller;
	QAction* show_action;
	
	Q_DISABLE_COPY(SymbolReportFeature)
};
	
}  // namespace OpenOrienteering

#endif  // OPENORIENTEERING_SYMBOL_REPORT_FEATURE_H
