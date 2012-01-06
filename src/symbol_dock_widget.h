/*
 *    Copyright 2011 Thomas Schöps
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


#ifndef _OPENORIENTEERING_SYMBOL_DOCK_WIDGET_H_
#define _OPENORIENTEERING_SYMBOL_DOCK_WIDGET_H_

#include <QWidget>

QT_BEGIN_NAMESPACE
class QScrollBar;
class QHBoxLayout;
class QMenu;
QT_END_NAMESPACE

class Map;
class SymbolWidget;

class SymbolRenderWidget : public QWidget
{
Q_OBJECT
public:
	SymbolRenderWidget(QScrollBar* scroll_bar, SymbolWidget* parent);
	
	inline bool scrollBarNeeded(int width, int height);
	void setScrollBar(QScrollBar* new_scroll_bar);
	
public slots:
	inline void setScroll(int new_scroll);
	
protected:
	QScrollBar* scroll_bar;
	QMenu* context_menu;
	
	void mouseMove(int x, int y);
	
	virtual void paintEvent(QPaintEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void wheelEvent(QWheelEvent* event);
};

class SymbolWidget : public QWidget
{
Q_OBJECT
public:
	SymbolWidget(Map* map, QWidget* parent = NULL);
	virtual ~SymbolWidget();
	
    virtual QSize sizeHint() const;
	
protected:
    virtual void resizeEvent(QResizeEvent* event);
	
protected slots:
	void newPointSymbol();
	void deleteSymbol();
	void duplicateSymbol();
	
private:
	SymbolRenderWidget* render_widget;
	QScrollBar* scroll_bar;
	
	QHBoxLayout* layout;
	bool no_resize_handling;
	QSize preferred_size;
	
	Map* map;
};

#endif
