/*
 * Copyright (c) 2005-2019 Libor Pecháček.
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

#include <QDialog>
#include <QImage>
#include <QList>
#include <QTranslator>

#include "Settings.h"
#include "Thread.h"
#include "libvectorizer/Vectorizer.h"
#include "templates/template.h"
#include "ui_mainform.h"

namespace OpenOrienteering {
class Map;
class TemplateImage;
} // namespace OpenOrienteering

namespace cove {
class ProgressObserver;
class UIProgressDialog;
class Vectorizer;

class mainForm : public QDialog
{
	Q_OBJECT

public:
	Ui::mainForm ui;

protected:
	UIProgressDialog* progressDialog;
	std::unique_ptr<Vectorizer> vectorizerApp;
	ClassificationThread* ct;
	OpenOrienteering::Map* ooMap;
	OpenOrienteering::Template* ooTempl;
	QString imageFileName;
	QImage imageBitmap;
	QImage classifiedBitmap;
	QImage bwBitmap;
	bool rollbackHistory;
	QList<QImage> bwBitmapHistory;
	QList<QImage>::iterator bwBitmapHistoryIterator;
	std::vector<QPushButton*> colorButtons;
	Settings settings;
	static const int MESSAGESHOWDELAY;

	bool performMorphologicalOperation(Vectorizer::MorphologicalOperation mo);
	void prepareBWImageHistory();
	void bwImageCommitHistory();
	void bwImageClearHistory();
	void clearColorsTab();
	void clearBWImageTab();
	void setTabEnabled(QWidget* tab, bool state);
	void afterLoadImage();
	QRgb getColorFromImage(const QImage& image);

public:
	mainForm(QWidget* parent, OpenOrienteering::Map* map,
			 OpenOrienteering::TemplateImage* templ, Qt::WindowFlags flags = 0);
	~mainForm();
	void loadImage(const QImage& imageToLoad, const QString& imageName);
	void clearColorButtonsGroup();
	void setColorButtonsGroup(std::vector<QRgb> colors);
	std::vector<bool> getSelectedColors();
	// unused slots
	void on_runOpeningButton_clicked();
	void on_runClosingButton_clicked();

public slots:
	void aboutDialog();
	void classificationFinished();
	void setInitialColors(bool on);
	void colorButtonToggled(bool on);

	void on_bwImageSaveButton_clicked();
	void on_classificationOptionsButton_clicked();
	void on_initialColorsButton_clicked();
	void on_howManyColorsSpinBox_valueChanged(int n);
	void on_mainTabWidget_currentChanged(int w);
	void on_setVectorizationOptionsButton_clicked();
	void on_createVectorsButton_clicked();
	void on_saveVectorsButton_clicked();
	void on_runClassificationButton_clicked();
	void on_runErosionButton_clicked();
	void on_runDilationButton_clicked();
	void on_runThinningButton_clicked();
	void on_runPruningButton_clicked();
	void on_bwImageHistoryBack_clicked();
	void on_bwImageHistoryForward_clicked();
	void on_applyFIRFilterPushButton_clicked();
};
} // cove

#endif
