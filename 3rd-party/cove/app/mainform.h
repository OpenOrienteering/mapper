/*
 * Copyright (c) 2005-2020 Libor Pecháček.
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

#ifndef COVE_MAINFORM_H
#define COVE_MAINFORM_H

#include <memory>
#include <vector>

#include <QDialog>
#include <QImage>
#include <QObject>
#include <QRgb>
#include <QString>
#include <Qt>
#include <QUndoCommand>

#include "libvectorizer/Vectorizer.h"

#include "Settings.h"
#include "ui_mainform.h"

class QPushButton;
class QWidget;
class QUndoStack;

namespace OpenOrienteering {
class Map;
class Template;
class TemplateImage;
} // namespace OpenOrienteering

namespace cove {

class mainForm : public QDialog
{
	Q_OBJECT

	struct BwBitmapUndoStep : public QUndoCommand
	{
		BwBitmapUndoStep(mainForm& form, QImage image, bool vectorizable);
		void redo() override;
		void undo() override;

		mainForm& form;
		QImage image;
		bool suitableForVectorization;
	};

public:
	Ui::mainForm ui;

protected:
	std::unique_ptr<Vectorizer> vectorizerApp;
	OpenOrienteering::Map* ooMap;
	OpenOrienteering::Template* ooTempl;
	QString imageFileName;
	QImage imageBitmap;
	QImage classifiedBitmap;
	QImage bwBitmap;
	bool bwBitmapVectorizable {};
	QUndoStack* bwBitmapUndo {};
	std::vector<QPushButton*> colorButtons;
	Settings settings;
	static const int MESSAGESHOWDELAY;

	bool performMorphologicalOperation(Vectorizer::MorphologicalOperation mo);
	void clearColorsTab();
	void clearBWImageTab();
	void setTabEnabled(QWidget* tab, bool state);
	void afterLoadImage();
	QRgb getColorFromImage(const QImage& image);

public:
	mainForm(QWidget* parent, OpenOrienteering::Map* map,
	         OpenOrienteering::TemplateImage* templ, Qt::WindowFlags flags = {});
	~mainForm() override;
	void loadImage(const QImage& imageToLoad, const QString& imageName);
	void clearColorButtonsGroup();
	void setColorButtonsGroup(std::vector<QRgb> colors);
	std::vector<bool> getSelectedColors();

public slots:
	void setInitialColors(bool on);
	void colorButtonToggled(bool on);

	void on_bwImageSaveButton_clicked();        // clazy:exclude=connect-by-name
	void on_classificationOptionsButton_clicked();  // clazy:exclude=connect-by-name
	void on_initialColorsButton_clicked();      // clazy:exclude=connect-by-name
	void on_howManyColorsSpinBox_valueChanged(int n);  // clazy:exclude=connect-by-name
	void on_mainTabWidget_currentChanged(int w);  // clazy:exclude=connect-by-name
	void on_setVectorizationOptionsButton_clicked();  // clazy:exclude=connect-by-name
	void on_createVectorsButton_clicked();      // clazy:exclude=connect-by-name
	void on_saveVectorsButton_clicked();        // clazy:exclude=connect-by-name
	void on_runClassificationButton_clicked();  // clazy:exclude=connect-by-name
	void on_runErosionButton_clicked();         // clazy:exclude=connect-by-name
	void on_runDilationButton_clicked();        // clazy:exclude=connect-by-name
	void on_runThinningButton_clicked();        // clazy:exclude=connect-by-name
	void on_runPruningButton_clicked();         // clazy:exclude=connect-by-name
	void on_bwImageHistoryBack_clicked();       // clazy:exclude=connect-by-name
	void on_bwImageHistoryForward_clicked();    // clazy:exclude=connect-by-name
	void on_applyFIRFilterPushButton_clicked(); // clazy:exclude=connect-by-name
};
} // cove

#endif
