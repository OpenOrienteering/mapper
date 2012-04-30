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
#include "main_window.h"

void SettingsPage::apply(){
	QSettings settings;
	for(int i = 0; i < changes.size(); i++){
		QPair<QString, QVariant> pair = changes.at(i);
		settings.setValue(pair.first, pair.second);
	}
}

////////////////////////////

SizePage::SizePage(MainWindow* main_window, QWidget* parent) : SettingsPage(main_window, parent){
	QHBoxLayout *l = new QHBoxLayout;
	this->setLayout(l);
	QPushButton *clear = new QPushButton(tr("Reset window size parameters"), this);
	l->addWidget(clear);
	connect(clear, SIGNAL(clicked()), this, SLOT(clear()));
}

void SizePage::clear(){
	int res = QMessageBox::question(this, tr("Confirm"), tr("This action will reset all window size parameters. Continue?"),
									QMessageBox::Yes | QMessageBox::No);
	if(res == QMessageBox::Yes){
		QSettings settings;
		settings.remove("ColorWidget/size");
		settings.remove("MainWindow/size");
		settings.remove("MainWindow/pos");
		settings.remove("MainWindow/maximized");
		settings.remove("SymbolWidget/size");
		settings.remove("TemplateWidget/size");
		QMessageBox::information(this, tr("Information"), tr("You will have to restart OpenOrienteering Mapper for changes to take effect"));
	}
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

	pages.append(new SizePage(main_window, this));
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
