/*
 *    Copyright 2012 Thomas Schöps
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


#ifndef _OPENORIENTEERING_MAIN_WINDOW_CONTROLLER_H_
#define _OPENORIENTEERING_MAIN_WINDOW_CONTROLLER_H_

#include <QWidget>

class MainWindow;


/** A MainWindowController provides the specific content and 
 *  behaviours for a main window.
 */
class MainWindowController : public QObject
{
Q_OBJECT
public:
	
	virtual ~MainWindowController() {}
	
	/** Save to a file.
	 *  @param path the path to save to
	 *  @return true if saving was sucessful, false on errors
	 */
	virtual bool save(const QString& path) {return false;}

	/** Load from a file.
	 *  @param path the path to load from
	 *  @return true if loading was sucessful, false on errors
	 */
	virtual bool load(const QString& path) {return false;}
	
	/** Attach the controller to a main window. 
	 *  The controller should create its user interface here.
	 */
	virtual void attach(MainWindow* window) = 0;
	
	/** Detach the controller from a main window. 
	 *  The controller should delete its user interface here.
	 */
	virtual void detach() {}
	
	// Get key press events from the main window
	virtual void keyPressEvent(QKeyEvent* event) {}
	virtual void keyReleaseEvent(QKeyEvent* event) {}
	
	/** Get the main window this controller is attached to.
	 */
	inline MainWindow* getWindow() const {return window;}
	
	/** Get a controller suitable for a particular file.
	 *  @param filename the name of the file
	 *  @return a MainWindowController that is able to load the file
	 */
	static MainWindowController* controllerForFile(const QString& filename);
	
protected:
	
	MainWindow* window;
};

#endif
