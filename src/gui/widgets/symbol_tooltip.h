/*
 *    Copyright 2012, 2013 Thomas Schöps
 *    Copyright 2017 Kai Pastor
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


#ifndef OPENORIENTEERING_SYMBOL_TOOLTIP_H
#define OPENORIENTEERING_SYMBOL_TOOLTIP_H

#include <QObject>
#include <QRect>
#include <QTimer>
#include <QWidget>

class QEvent;
class QHideEvent;
class QLabel;
class QPaintEvent;
class QShortcut;
class QShowEvent;
// IWYU pragma: no_forward_declare QWidget

namespace OpenOrienteering {

class Map;
class Symbol;


/**
 * @brief A SymbolToolTip displays the name and description of a symbol.
 * 
 * The tooltip is normally not displayed immediately but scheduled to be
 * displayed after a short delay, by a call to scheduleShow(). Initially, the
 * tooltip displays the symbol's number and name only. The description can be
 * made visible by a call to showDescription().
 * 
 * A QShortcut may be set which will trigger the display of the description
 * while the tooltip is already visible. The shortcut does not need to be
 * unique: SymbolToolTip enables the shortcut only when the tooltip is visible
 * and the description is not yet shown. SymbolToolTip does not take ownership
 * of the shortcut.
 * 
 * To permanently hide a SymbolToolTip it is not enough to call hide() because
 * the widget may be hidden but scheduled to show up. Call reset() to hide the
 * widget and stop the timer if scheduled.
 * 
 * SymbolToolTip does not use the standard tooltip colors but the text editor
 * colors.
 */
class SymbolToolTip : public QWidget
{
Q_OBJECT
public:
	/**
	 * Constructs a new SymbolToolTip.
	 * The optional shortcut will trigger the description to be shown.
	 */
	SymbolToolTip(QWidget* parent = nullptr, QShortcut* shortcut = nullptr);
	
	/**
	 * Schedules the tooltip for the symbol to be shown close to
	 * but not covering the region given by rect.
	 * 
	 * In mobile mode, tooltip placement relative to the symbol icon is tried
	 * in the order: above, left, right, below.
	 * Otherwise, placement is tried in the order: below, above, right, left.
	 * (desktop mode).
	 */
	void scheduleShow(const Symbol* symbol, const Map* map, QRect rect, bool mobile_mode);
	
	/**
	 * Resets the tooltip.
	 * It hides the widget, stops the timer and disables the shortcut.
	 */
	void reset();
	
	/**
	 * Returns the symbol for which the tooltip is currently shown or
	 * scheduled to be shown, or nullptr.
	 */
	const Symbol* getSymbol() const;
	
public slots:
	/**
	 * Expands the symbol's description in the tooltip.
	 * Disables the shortcut.
	 */
	void showDescription();
	
protected:
	/**
	 * Hides the tooltip when the mouse enters it.
	 * This is necessary to let the user select another symbol.
	 */
	void enterEvent(QEvent* event) override;
	
	/**
	 * Enables the shortcut when the tooltip is shown.
	 */
	void showEvent(QShowEvent* event) override;
	
	/**
	 * Resets the tooltip's state on hiding the tooltip.
	 * Disables the shortcut.
	 */
	void hideEvent(QHideEvent* event) override;
	
	/**
	 * Draws the tooltip's background.
	 */
	void paintEvent(QPaintEvent* event) override;
	
private:
	/**
	 * Moves the tooltip so that it is nicely placed close the region given
	 * to scheduleShow().
	 */
	void adjustPosition(bool mobile_mode);
	
	QTimer tooltip_timer;      /// The timer which triggers the delayed showing.
	QShortcut* shortcut;       /// An optional shortcut for showing the description.
	const Symbol* symbol;      /// The current symbol, or nullptr.
	QLabel* name_label;        /// The label displaying the symbol's name.
	QLabel* description_label; /// The label displaying the symbol's description.
	QRect icon_rect;           /// The region to be considered when determining position.
	bool description_shown;    /// If true, the full description is visible.
	
};


}  // namespace OpenOrienteering

#endif
