#include "settings_dialog.h"

#include <QHeaderView>
#include <QEvent>
#include <QLineEdit>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QHBoxLayout>
#include <QDebug>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QRadioButton>
#include "map_editor.h"
#include "map_widget.h"
#include "main_window.h"

void SettingsPage::apply(){
	QSettings settings;
	for(int i = 0; i < changes.size(); i++){
		settings.setValue(changes.keys().at(i), changes.values().at(i));
	}
}

////////////////////////////

RenderPage::RenderPage(MainWindow* main_window, QWidget* parent) : SettingsPage(main_window, parent){
	QHBoxLayout *l = new QHBoxLayout;
	this->setLayout(l);
	QRadioButton *antialiasing = new QRadioButton(tr("Enable Antialiasing"), this);
	QRadioButton *noantialiasing = new QRadioButton(tr("Disable Antialiasing"), this);
	l->addWidget(antialiasing);
	l->addWidget(noantialiasing);

	QSettings current;
	if(current.value("MapDisplay/antialiasing", QVariant(true)).toBool())
		antialiasing->click();
	else
		noantialiasing->click();

	connect(antialiasing, SIGNAL(clicked()), this, SLOT(antialiasingClicked()));
	connect(noantialiasing, SIGNAL(clicked()), this, SLOT(noantialiasingClicked()));
}

void RenderPage::antialiasingClicked(){
	changes.insert("MapDisplay/antialiasing", true);
}
void RenderPage::noantialiasingClicked(){
	changes.insert("MapDisplay/antialiasing", false);
}

void RenderPage::apply(){
	SettingsPage::apply();
	QSettings settings;
	bool use = settings.value("MapDisplay/antialiasing", QVariant(true)).toBool();
	if(qobject_cast<MapEditorController*>(main_window->getController()))
		qobject_cast<MapEditorController*>(main_window->getController())->getMainWidget()->setUsesAntialiasing(use);
}

////////////////////////////

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
	tabWidget = new QTabWidget(this);
	QVBoxLayout *l = new QVBoxLayout;
	l->addWidget(tabWidget);
	this->setLayout(l);
	this->resize(640, 480);

	dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel,
												 Qt::Horizontal, this);
	l->addWidget(dbb);
	connect(dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonPressed(QAbstractButton*)));

	MainWindow *main_window = qobject_cast<MainWindow*>(parent);
	if(!main_window){
		this->reject();
		return;
	}

	pages.append(new RenderPage(main_window, this));
	tabWidget->addTab(pages.last(), pages.last()->title());
}

void SettingsDialog::buttonPressed(QAbstractButton *b){
	QDialogButtonBox::StandardButton button = dbb->standardButton(b);
	if(button == QDialogButtonBox::Ok){
		foreach(SettingsPage *page, pages){
			page->ok();
		}
		this->accept();
	}
	else if(button == QDialogButtonBox::Apply){
		foreach(SettingsPage *page, pages){
			page->apply();
		}
	}
	else if(button == QDialogButtonBox::Cancel){
		foreach(SettingsPage *page, pages){
			page->cancel();
		}
		this->reject();
	}
}

#include "settings_dialog.moc"
