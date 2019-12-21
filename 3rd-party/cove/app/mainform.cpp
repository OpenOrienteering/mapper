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

#include <stdlib.h>
#include <time.h>

#include <stdexcept>

#include <QEvent>
#include <QFileDialog>
#include <QImage>
#include <QImageReader>
#include <QList>
#include <QListIterator>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObjectList>
#include <QPainter>
#include <QPixmap>

#include "core/map.h"
#include "core/objects/object.h"
#include "core/symbols/line_symbol.h"
#include "templates/template_image.h"
#include "undo/object_undo.h"

#include "app/FIRFilter.h"
#include "app/Polygons.h"
#include "app/Thread.h"
#include "app/UIProgressDialog.h"
#include "app/classificationconfigform.h"
#include "app/colorseditform.h"
#include "app/mainform.h"
#include "app/vectorizationconfigform.h"
#include "libvectorizer/Vectorizer.h"
#include "ui_mainform.h"

using namespace std;
namespace cove {
//@{
/*! \defgroup gui GUI classes */

/*! \class mainForm
  \brief Main form that is used as centralWidget() of QMainWindow.
  */

/*! \var Ui::mainForm mainForm::ui
  User interface as created by Qt Designer.
  */

/*! \var UIProgressDialog* mainForm::progressDialog
  Progress dialog pointer. Is 0 when there is no progress dialog currently
  displayed.
*/

/*! \var Vectorizer* mainForm::vectorizerApp
  Vectorizer application.
  \sa Vectorizer
*/

/*! \var QString mainForm::imageFileName
  Filename of the currently loaded image.
*/

/*! \var QImage mainForm::imageBitmap
  The currently loaded image.
*/

/*! \var QImage mainForm::classifiedBitmap
  The classified image on the colors tab.
*/

/*! \var QImage mainForm::bwBitmap
  The BW image on the thinning tab.
*/

/*! \var bool mainForm::rollbackHistory
  Boolean indicating whether the history was commited.
  \sa prepareBWImageHistory, commitBWImageHistory
*/

/*! \var QList<QImage> mainForm::bwBitmapHistory
  Container for all the images in the history.
*/

/*! \var QList<QImage>::iterator mainForm::bwBitmapHistoryIterator
  Pointer to \a bwBitmapHistory.
*/

/*! \var int mainForm::numberOfColorButtons
  Number of color buttons in the ColorButtonsGroup on the Colors tab.
  \sa colorButtons
*/

/*! \var QPushButton** mainForm::colorButtons
  Array of pointers to color buttons in the ColorButtonsGroup on the Colors tab.
  \sa numberOfColorButtons
*/

/*! \var Settings mainForm::settings
  Current status data (configuraiton variables).
*/

/*! \var QDoubleValidator mainForm::positiveDoubleValid
  Input validator for LineEdit.
*/

//! Constructor initializing the menu, actions and connecting signals from
//! actions
mainForm::mainForm(QWidget* parent, OpenOrienteering::Map* map,
				   OpenOrienteering::TemplateImage* templ,
				   Qt::WindowFlags flags)
	: QDialog(parent, flags)
	, ooMap(map)
	, ooTempl(templ)
{
	ui.setupUi(this);

	vectorizerApp = nullptr;
	progressDialog = nullptr;

	setTabEnabled(ui.imageTab, true);
	setTabEnabled(ui.thinningTab, false);
	setTabEnabled(ui.colorsTab, false);

	ui.howManyColorsSpinBox->setValue(settings.getInt("nColors"));
	rollbackHistory = false;
	bwBitmapHistoryIterator = bwBitmapHistory.begin();

	loadImage(templ->getImage(), templ->getTemplateFilename());
}

mainForm::~mainForm()
{
	if (progressDialog) progressDialog->deleteLater();
	clearColorButtonsGroup();
}

//! Clears the Thinning tab, i.e. removes displayed image and polygons
void mainForm::clearBWImageTab()
{
	bwImageClearHistory();
	ui.bwImageView->setPolygons(Polygons::PolygonList());
	ui.saveVectorsButton->setEnabled(false);
	ui.bwImageView->setImage(nullptr);
}

//! Clears the Colors tab, i.e. removes displayed image and color buttons
void mainForm::clearColorsTab()
{
	if (!imageBitmap.isNull())
	{
		ui.classifiedColorsView->setImage(&imageBitmap);
	}
	else
	{
		ui.classifiedColorsView->setImage(nullptr);
	}
	clearColorButtonsGroup();
}

//! Loads image into form.
void mainForm::loadImage(const QImage& imageToLoad, const QString& imageName)
{
	imageFileName = imageName;

	// no alpha channel desired in the vectorized image
	// we also want to have white background to mimic Mapper look and feel
	imageBitmap = QImage(imageToLoad.size(), QImage::Format_RGB32);
	imageBitmap.fill(Qt::white);
	QPainter painter(&imageBitmap);
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.drawImage(0, 0, imageToLoad);
	painter.end();

	afterLoadImage();
}

//! Performs form modifications after image load.
void mainForm::afterLoadImage()
{
	ui.mainTabWidget->setCurrentIndex(ui.mainTabWidget->indexOf(ui.imageTab));
	setTabEnabled(ui.thinningTab, false);
	clearBWImageTab();
	setTabEnabled(ui.colorsTab, false);
	clearColorsTab();
	ui.imageView->setImage(&imageBitmap);
	ui.imageView->reset();
	ui.classifiedColorsView->setImage(&imageBitmap);
	ui.classifiedColorsView->reset();

	if (!imageBitmap.isNull())
	{
		if (imageBitmap.format() == QImage::Format_Mono ||
			imageBitmap.format() == QImage::Format_MonoLSB)
		{
			setTabEnabled(ui.thinningTab, true);
		}
		setTabEnabled(ui.colorsTab, true);
	}
}

//! Simple About dialog.
void mainForm::aboutDialog()
{
	QMessageBox::about(this, tr("About"),
					   trUtf8("<h3>COntour VEctorizer</h3><br />"
							  "Author: Libor Pecháček<br />"
							  "License: GPL"));
}

/*! Return color randomly created from image pixels.  It takes 5 scanlines and
 * makes an average color of its pixels. */
QRgb mainForm::getColorFromImage(const QImage& image)
{
	srand(time(nullptr));
	unsigned long r, g, b, divisor;
	r = g = b = divisor = 0;
	for (int a = 0; a < 5; a++)
	{
		int line = rand() % image.height();
		int w = image.width();
		for (int x = 0; x < w; x++)
		{
			QRgb c = image.pixel(x, line);
			r += qRed(c);
			g += qGreen(c);
			b += qBlue(c);
			divisor++;
		}
	}
	return qRgb(r / divisor, g / divisor, b / divisor);
}

/*! Performs actions requested by clicking 'Now' button on Colors tab.
  \sa classificationFinished() */
void mainForm::on_runClassificationButton_clicked()
{
	ui.runClassificationButton->setEnabled(false);
	vectorizerApp = std::make_unique<Vectorizer>(imageBitmap);
	progressDialog = new UIProgressDialog(tr("Colors classification in "
											 "progress"),
										  tr("Cancel"), this);
	switch (settings.getInt("learnMethod"))
	{
	case 0:
		vectorizerApp->setClassificationMethod(Vectorizer::KOHONEN_CLASSIC);
		break;
	case 1:
		vectorizerApp->setClassificationMethod(Vectorizer::KOHONEN_BATCH);
		break;
	}
	vectorizerApp->setP(settings.getDouble("p"));
	switch (settings.getInt("colorSpace"))
	{
	case 0:
		vectorizerApp->setColorSpace(Vectorizer::COLSPC_RGB);
		break;
	case 1:
		vectorizerApp->setColorSpace(Vectorizer::COLSPC_HSV);
		break;
	}
	switch (settings.getInt("alphaStrategy"))
	{
	case 0:
		vectorizerApp->setAlphaStrategy(Vectorizer::ALPHA_CLASSIC);
		break;
	case 1:
		vectorizerApp->setAlphaStrategy(Vectorizer::ALPHA_NEUQUANT);
		break;
	}
	switch (settings.getInt("patternStrategy"))
	{
	case 0:
		vectorizerApp->setPatternStrategy(Vectorizer::PATTERN_RANDOM);
		break;
	case 1:
		vectorizerApp->setPatternStrategy(Vectorizer::PATTERN_NEUQUANT);
		break;
	}
	vectorizerApp->setInitAlpha(settings.getDouble("initAlpha"));
	vectorizerApp->setMinAlpha(settings.getDouble("minAlpha"));
	vectorizerApp->setQ(settings.getDouble("q"));
	vectorizerApp->setE(settings.getInt("E"));
	int nColors = settings.getInt("nColors");
	vectorizerApp->setNumberOfColors(nColors);
	std::vector<QRgb> colors;
	switch (settings.getInt("initColorsSource"))
	{
	case ColorsEditForm::Random:
		break;
	case ColorsEditForm::RandomFromImage:
		colors.resize(nColors);
		for (auto& c : colors)
			c = getColorFromImage(imageBitmap);
		break;
	case ColorsEditForm::Predefined:
		colors = settings.getInitColors();
		break;
	}
	vectorizerApp->setInitColors(colors);
	ct = new ClassificationThread(*vectorizerApp, progressDialog, this);
	connect(ct, SIGNAL(finished()), this, SLOT(classificationFinished()));
	ct->start();
}

//! Second part of on_runClassificationButton_clicked(), performs actions after
// the classification is done, i.e. creates classified image if necessary and
// creates color buttons by calling \a setColorButtonsGroup.
void mainForm::classificationFinished()
{
	auto colorsFound = vectorizerApp->getClassifiedColors();
	setColorButtonsGroup(colorsFound);
	progressDialog->percentageChanged(100);
	progressDialog->deleteLater();
	progressDialog = nullptr;
	ct->deleteLater();
	ct = nullptr;

	// in case there are some colors (learning was not aborted)
	if (colorsFound.size())
	{
		double quality;
		progressDialog = new UIProgressDialog(tr("Creating classified image"),
											  tr("Cancel"), this);
		QImage newClassifiedBitmap =
			vectorizerApp->getClassifiedImage(&quality, progressDialog);
		progressDialog->percentageChanged(100);
		progressDialog->deleteLater();
		progressDialog = nullptr;
		if (!newClassifiedBitmap.isNull())
		{
			classifiedBitmap = newClassifiedBitmap;
			ui.classifiedColorsView->setImage(&classifiedBitmap);
			ui.learnQualityLabel->setText(QString("LQ: %1").arg(quality));
		}
		else
		{
			clearColorButtonsGroup();
		}
	}
	else
	{
		ui.learnQualityLabel->setText(QString("LQ: -"));
	}
	ui.runClassificationButton->setEnabled(true);
}

/*! Removes all color buttons from the frame on Colors tab.
  \sa setColorButtonsGroup(QRgb* colors, int nColors) */
void mainForm::clearColorButtonsGroup()
{
	for (auto button : colorButtons)
	{
		disconnect(button);
		ui.gridLayout->removeWidget(button);
		button->deleteLater();
	}
	colorButtons.clear();
}

/*! Creates color buttons in the frame on Colors tab.
  \sa clearColorButtonsGroup() */
void mainForm::setColorButtonsGroup(std::vector<QRgb> colors)
{
	clearColorButtonsGroup();
	auto nColors = colors.size();
	if (!nColors) return;
	colorButtons.resize(nColors + 1);
	unsigned i;
	for (i = 0; i < nColors; i++)
	{
		QPixmap buttonFace(20, 20);
		buttonFace.fill(QColor(colors[i]));
		QPushButton* button =
			new QPushButton(buttonFace, QString(), ui.colorButtonsGroup);
		ui.gridLayout->addWidget(button, i % 2, i / 2);
		colorButtons[i] = button;
		button->setCheckable(true);
		button->setMaximumSize(30, 30);
		button->setMinimumSize(10, 10);
		button->show();
		connect(button, SIGNAL(toggled(bool)), this,
				SLOT(colorButtonToggled(bool)));
	}
	QPushButton* button = new QPushButton(ui.colorButtonsGroup);
	ui.gridLayout->addWidget(button, i % 2, i / 2);
	colorButtons[i] = button;
	button->setMaximumSize(30, 30);
	button->setMinimumSize(10, 10);
	button->setToolTip(tr("Set these colors as initial."));
	button->setText(tr("IC"));
	button->show();
	connect(button, SIGNAL(clicked(bool)), this, SLOT(setInitialColors(bool)));
	ui.colorButtonsGroup->adjustSize();
}

//! Performs action requested by clicking IC button in the color buttons frame,
// i.e. sets current colors as initial.
void mainForm::setInitialColors(bool /*on*/)
{
	auto colorsFound = vectorizerApp->getClassifiedColors();
	auto n = colorsFound.size();
	if (!n) return;
	auto comments = std::vector<QString>(n);
	settings.setInitColors(colorsFound, comments);
}

//! Invoked by clicking a color button in the color buttons frame, changes the
// classified bitmap palette so that pixels of current selected colors appear in
// dark orange.
void mainForm::colorButtonToggled(bool /*on*/)
{
	int sels = 0;
	auto colorsFound = vectorizerApp->getClassifiedColors();
	auto selectedColors = getSelectedColors();
	for (std::size_t i = 0; i < colorButtons.size(); i++)
	{
		classifiedBitmap.setColor(
			i, selectedColors[i] ? (sels++, qRgb(255, 27, 0)) : colorsFound[i]);
	}
	setTabEnabled(ui.thinningTab, sels != 0);
	ui.classifiedColorsView->setImage(&classifiedBitmap);
	ui.classifiedColorsView->update();
}

//! Returns bool array indicating which colors are selected by color buttons.
std::vector<bool> mainForm::getSelectedColors()
{
	auto n = vectorizerApp->getClassifiedColors().size();
	// do not allow different number of colors and buttons
	Q_ASSERT(colorButtons.size() >= n);

	auto retval = std::vector<bool>(n);
	// copy states of the buttons into array
	for (std::size_t i = 0; i < colorButtons.size(); i++)
		retval[i] = colorButtons[i]->isChecked();

	return retval;
}

//! Performs generating the BW image on change to Thinning tab.
void mainForm::on_mainTabWidget_currentChanged(int tabindex)
{
	if (ui.mainTabWidget->widget(tabindex) != ui.thinningTab) return;
	clearBWImageTab();
	if ((imageBitmap.format() == QImage::Format_Mono ||
		 imageBitmap.format() == QImage::Format_MonoLSB) &&
		colorButtons.empty())
	{
		bwBitmap = imageBitmap;
		ui.bwImageView->setImage(&bwBitmap);
		ui.bwImageView->reset();
		return;
	}
	auto selectedColors = getSelectedColors();
	progressDialog =
		new UIProgressDialog(tr("Creating B/W image"), tr("Cancel"), this);
	QImage newBWBitmap =
		vectorizerApp->getBWImage(selectedColors, progressDialog);
	progressDialog->percentageChanged(100);
	progressDialog->deleteLater();
	progressDialog = nullptr;
	if (!newBWBitmap.isNull())
	{
		bwBitmap = newBWBitmap;
		ui.bwImageView->setImage(&bwBitmap);
		ui.bwImageView->reset();
		setTabEnabled(ui.thinningTab, true);
	}
	else
	{
		setTabEnabled(ui.thinningTab, false);
	}
}

//! Performs one morphological operation on the bwImage.
bool mainForm::performMorphologicalOperation(
	Vectorizer::MorphologicalOperation mo)
{
	QString text;
	switch (mo)
	{
	case Vectorizer::THINNING_ROSENFELD:
		text = tr("Thinning B/W image");
		break;
	case Vectorizer::PRUNING:
		text = tr("Pruning B/W image");
		break;
	case Vectorizer::EROSION:
		text = tr("Eroding B/W image");
		break;
	case Vectorizer::DILATION:
		text = tr("Dilating B/W image");
		break;
	}
	progressDialog = new UIProgressDialog(text, tr("Cancel"), this);
	QImage transBitmap =
		Vectorizer::getTransformedImage(bwBitmap, mo, progressDialog);
	progressDialog->percentageChanged(100);
	progressDialog->deleteLater();
	progressDialog = nullptr;

	if (!transBitmap.isNull())
	{
		bwBitmap = transBitmap;
		ui.bwImageView->setImage(&bwBitmap);
		return true;
	}
	else
	{
		return false;
	}
}

//! Inserts the current displayed image into the history queue.  Pops the last
// image from the history queue in case \a rollbackHistory is set, what means
// the last image was not commited into the queue.
void mainForm::prepareBWImageHistory()
{
	if (rollbackHistory)
	{
		if (!bwBitmapHistory.empty()) bwBitmapHistory.pop_front();
	}
	else if (bwBitmapHistoryIterator != bwBitmapHistory.begin())
	{
		bwBitmapHistory.erase(bwBitmapHistory.begin(), bwBitmapHistoryIterator);
		bwBitmapHistory.pop_front();
	}
	bwBitmapHistory.push_front(bwBitmap);
	bwBitmapHistoryIterator = bwBitmapHistory.begin();
	ui.bwImageView->setImage(&bwBitmap);
	rollbackHistory = true;
}

//! Completely clears the history.
void mainForm::bwImageClearHistory()
{
	ui.bwImageHistoryBack->setEnabled(false);
	ui.bwImageHistoryForward->setEnabled(false);
	bwBitmapHistory.clear();
	bwBitmapHistoryIterator = bwBitmapHistory.begin();
	rollbackHistory = false;
}

//! Commits the image into the history queue.
void mainForm::bwImageCommitHistory()
{
	ui.bwImageHistoryBack->setEnabled(true);
	ui.bwImageHistoryForward->setEnabled(false);
	rollbackHistory = false;
}

//! Returns one image back in the history.
void mainForm::on_bwImageHistoryBack_clicked()
{
	// safety check
	if (bwBitmapHistoryIterator == bwBitmapHistory.end()) return;
	if (bwBitmapHistoryIterator == bwBitmapHistory.begin())
	{
		bwBitmapHistory.push_front(bwBitmap);
		bwBitmapHistoryIterator = bwBitmapHistory.begin();
	}
	if (++bwBitmapHistoryIterator != bwBitmapHistory.end())
	{
		bwBitmap = *bwBitmapHistoryIterator;
		ui.bwImageView->setImage(&bwBitmap);
		if (bwBitmapHistoryIterator + 1 == bwBitmapHistory.end())
		{
			ui.bwImageHistoryBack->setEnabled(false);
		}
		ui.bwImageHistoryForward->setEnabled(true);
	}
}

//! Advances one image forward in the history.
void mainForm::on_bwImageHistoryForward_clicked()
{
	if (bwBitmapHistoryIterator != bwBitmapHistory.begin())
	{
		bwBitmap = *(--bwBitmapHistoryIterator);
		ui.bwImageView->setImage(&bwBitmap);
		ui.bwImageHistoryBack->setEnabled(true);
		if (bwBitmapHistoryIterator == bwBitmapHistory.begin())
		{
			ui.bwImageHistoryForward->setEnabled(false);
			bwBitmapHistory.pop_front();
			bwBitmapHistoryIterator = bwBitmapHistory.begin();
		}
	}
}

/*! Runs the thinning.
  \sa performMorphologicalOperation */
void mainForm::on_runThinningButton_clicked()
{
	prepareBWImageHistory();
	if (performMorphologicalOperation(Vectorizer::THINNING_ROSENFELD))
		bwImageCommitHistory();
}

/*! Runs the pruning.
  \sa performMorphologicalOperation */
void mainForm::on_runPruningButton_clicked()
{
	prepareBWImageHistory();
	if (performMorphologicalOperation(Vectorizer::PRUNING))
		bwImageCommitHistory();
}

/*! Erodes the image.
  \sa performMorphologicalOperation */
void mainForm::on_runErosionButton_clicked()
{
	prepareBWImageHistory();
	if (performMorphologicalOperation(Vectorizer::EROSION))
		bwImageCommitHistory();
}

/*! Dilates the image.
  \sa performMorphologicalOperation */
void mainForm::on_runDilationButton_clicked()
{
	prepareBWImageHistory();
	if (performMorphologicalOperation(Vectorizer::DILATION))
		bwImageCommitHistory();
}

/*! Opens the image.
  \sa performMorphologicalOperation */
void mainForm::on_runOpeningButton_clicked()
{
	prepareBWImageHistory();
	QImage savedimage = bwBitmap;
	if (performMorphologicalOperation(Vectorizer::EROSION) &&
		performMorphologicalOperation(Vectorizer::DILATION))
		bwImageCommitHistory();
	else
		bwBitmap = savedimage;
}

/*! Closes the image.
  \sa performMorphologicalOperation */
void mainForm::on_runClosingButton_clicked()
{
	prepareBWImageHistory();
	QImage savedimage = bwBitmap;
	if (performMorphologicalOperation(Vectorizer::DILATION) &&
		performMorphologicalOperation(Vectorizer::EROSION))
		bwImageCommitHistory();
	else
		bwBitmap = savedimage;
}

/*! Spawns dialog on classification config options.
  \sa ClassificationConfigForm */
void mainForm::on_classificationOptionsButton_clicked()
{
	ClassificationConfigForm* optionsDialog =
		new ClassificationConfigForm(this);
	optionsDialog->learnMethod = settings.getInt("learnMethod");
	optionsDialog->colorSpace = settings.getInt("colorSpace");
	optionsDialog->p = settings.getDouble("p");
	optionsDialog->E = settings.getInt("E");
	optionsDialog->Q = settings.getDouble("q");
	optionsDialog->initAlpha = settings.getDouble("initAlpha");
	optionsDialog->minAlpha = settings.getDouble("minAlpha");
	optionsDialog->setValues();
	optionsDialog->exec();
	if (optionsDialog->result() == ClassificationConfigForm::Accepted)
	{
		settings.setInt("E", optionsDialog->E);
		settings.setDouble("q", optionsDialog->Q);
		settings.setDouble("initAlpha", optionsDialog->initAlpha);
		settings.setDouble("minAlpha", optionsDialog->minAlpha);
		settings.setDouble("learnMethod", optionsDialog->learnMethod);
		settings.setDouble("colorSpace", optionsDialog->colorSpace);
		settings.setDouble("p", optionsDialog->p);
	}
	delete optionsDialog;
}

/*! Spawns initial colors selection dialog.
  \sa ColorsEditForm */
void mainForm::on_initialColorsButton_clicked()
{
	ColorsEditForm* optionsDialog = new ColorsEditForm(this);
	int settingsNColors = settings.getInt("nColors");
	std::vector<QString> comments;
	auto colors = settings.getInitColors(comments);
	auto nColors = colors.size();

	if (colors.empty())
	{
		// There are no settings in the store
		nColors = settingsNColors;
		colors = std::vector<QRgb>(nColors);
		comments = std::vector<QString>(nColors);
		for (std::size_t i = 0; i < nColors; i++)
		{
			colors[i] = qRgb(127, 127, 127);
			comments[i] = QString(tr("new color"));
		}
	}
	else if (nColors < settingsNColors)
	{
		// Existing settings only partially cover the higher number of colors
		colors.resize(settingsNColors, qRgb(255, 255, 255));
		comments.resize(settingsNColors, QString(tr("new color")));
	}

	optionsDialog->setColors(colors, comments);
	optionsDialog->setColorsSource(static_cast<ColorsEditForm::ColorsSource>(
		settings.getInt("initColorsSource")));
	optionsDialog->exec();
	if (optionsDialog->result() == ColorsEditForm::Accepted)
	{
		settings.setInt("initColorsSource", optionsDialog->getColorsSource());
		settings.setInitColors(optionsDialog->getColors(),
							   optionsDialog->getComments());
	}
	delete optionsDialog;
}

//! Stores the value of number of colors spinbox located at colors tab.
void mainForm::on_howManyColorsSpinBox_valueChanged(int n)
{
	settings.setInt("nColors", n);
}

//! Spawns dialog on vectorization options.
void mainForm::on_setVectorizationOptionsButton_clicked()
{
	VectorizationConfigForm optionsDialog(this);
	optionsDialog.speckleSize = settings.getDouble("speckleSize");
	optionsDialog.doConnections = settings.getInt("doConnections");
	optionsDialog.joinDistance = settings.getDouble("joinDistance");
	optionsDialog.simpleConnectionsOnly =
		settings.getInt("simpleConnectionsOnly");
	optionsDialog.distDirBalance = settings.getDouble("distDirBalance");
	optionsDialog.cornerMin = settings.getDouble("cornerMin");
	optionsDialog.optTolerance = settings.getDouble("optTolerance");
	optionsDialog.exec();
	if (optionsDialog.result() == ClassificationConfigForm::Accepted)
	{
		settings.setDouble("speckleSize", optionsDialog.speckleSize);
		settings.setDouble("joinDistance", optionsDialog.joinDistance);
		settings.setInt("doConnections", optionsDialog.doConnections);
		settings.setInt("simpleConnectionsOnly",
						optionsDialog.simpleConnectionsOnly);
		settings.setDouble("distDirBalance", optionsDialog.distDirBalance);
		settings.setDouble("cornerMin", optionsDialog.cornerMin);
		settings.setDouble("optTolerance", optionsDialog.optTolerance);
	}
}

//! Creates polygons from current bwImage.
void mainForm::on_createVectorsButton_clicked()
{
	Polygons::PolygonList* q = new Polygons::PolygonList;
	Polygons p;

	p.setSpeckleSize(settings.getInt("speckleSize"));
	p.setMaxDistance(settings.getInt("doConnections")
						 ? settings.getDouble("joinDistance")
						 : 0);
	p.setSimpleOnly(settings.getInt("simpleConnectionsOnly"));
	p.setDistDirRatio(settings.getDouble("distDirBalance"));
	progressDialog =
		new UIProgressDialog(tr("Vectorizing"), tr("Cancel"), this);
	*q = p.createPolygonsFromImage(bwBitmap, progressDialog);
	progressDialog->percentageChanged(100);
	progressDialog->deleteLater();
	progressDialog = nullptr;
	if (q->empty())
	{
		delete q;
		return;
	}

	ui.bwImageView->setPolygons(*q);
	ui.saveVectorsButton->setEnabled(true);
	delete q;
}

//! Transfers traced polygons back to the map.
void mainForm::on_saveVectorsButton_clicked()
{
	const Polygons::PolygonList polys = ui.bwImageView->polygons();
	if (polys.empty()) return;

	float xOff = float(-ui.bwImageView->image()->width()) / 2;
	float yOff = float(-ui.bwImageView->image()->height()) / 2;
	OpenOrienteering::DeleteObjectsUndoStep* undo_step =
		new OpenOrienteering::DeleteObjectsUndoStep(ooMap);
	OpenOrienteering::MapPart* part = ooMap->getCurrentPart();
	std::vector<OpenOrienteering::Object*> result;

	ooMap->clearObjectSelection(false);

	for (const auto& polygon : polys)
	{
		OpenOrienteering::PathObject* newOOPolygon =
			new OpenOrienteering::PathObject();
		newOOPolygon->setSymbol(ooMap->getUndefinedLine(), true);
		newOOPolygon->setTag(QStringLiteral("name"), QStringLiteral("cove"));

		for (const auto& point : polygon)
		{
			// transform from template coordinates to map coordinates
			OpenOrienteering::MapCoordF templCoords =
				ooTempl->templateToMap(QPoint(point.x + xOff, point.y + yOff));
			OpenOrienteering::MapCoord c(templCoords.x(), templCoords.y());

			newOOPolygon->addCoordinate(c);
		}

		if (polygon.isClosed()) newOOPolygon->closeAllParts();

		ooMap->addObject(newOOPolygon);
		ooMap->addObjectToSelection(newOOPolygon, false);
		result.push_back(newOOPolygon);
	}

	for (auto i : result)
		undo_step->addObject(part->findObjectIndex(i));
	ooMap->push(undo_step);

	ooMap->setObjectsDirty();
	ooMap->emitSelectionChanged();
}

//! Saves current bwImage to an image file.
void mainForm::on_bwImageSaveButton_clicked()
{
	QString filter;
	QList<QByteArray> extensions = QImageReader::supportedImageFormats();
	QList<QByteArray>::iterator it;

	for (it = extensions.begin(); it != extensions.end(); ++it)
	{
		filter.append(QString(*it).toUpper());
		filter.append(QString(*it).prepend(" (*.").append(");;"));
	}

	filter.append(tr("All files"));
	filter.append("(*)");

	QString selectedFilter("PNG (*.png)");
	QString newfilename = QFileDialog::getSaveFileName(
		this, QString::null, QFileInfo(imageFileName).dir().path(), filter,
		&selectedFilter);

	if (!newfilename.isEmpty())
	{
		QString suffix = selectedFilter.section(" ", 1, 1);
		suffix = suffix.mid(2, suffix.size() - 3);
		if (!newfilename.endsWith(suffix, Qt::CaseInsensitive))
			newfilename.append(suffix);
		bwBitmap.save(newfilename,
					  selectedFilter.section(" ", 0, 0).toLatin1().constData());
	}
}

//! Convenience method for enabling/disabling an UI tab.
void mainForm::setTabEnabled(QWidget* tab, bool state)
{
	int tabindex = ui.mainTabWidget->indexOf(tab);
	ui.mainTabWidget->setTabEnabled(tabindex, state);
}

/*! Applies FIR filter onto \a imageBitmap.
  \sa FIRFilter */
void mainForm::on_applyFIRFilterPushButton_clicked()
{
	if (imageBitmap.isNull()) return;
	FIRFilter f;
	switch (ui.firFilterComboBox->currentIndex())
	{
	case 0:
		f = FIRFilter(2).binomic();
		break;
	case 1:
		f = FIRFilter(3).binomic();
		break;
	case 2:
		f = FIRFilter(2).box();
		break;
	case 3:
		f = FIRFilter(3).box();
		break;
	}

	progressDialog = new UIProgressDialog(tr("Applying FIR Filter on image"),
										  tr("Cancel"), this);
	QImage newImageBitmap =
		f.apply(imageBitmap, qRgb(127, 127, 127), progressDialog);
	progressDialog->percentageChanged(100);
	progressDialog->deleteLater();
	progressDialog = nullptr;
	if (!newImageBitmap.isNull()) imageBitmap = newImageBitmap;
	ui.imageView->setImage(&imageBitmap);
}
} // cove

//@}
