#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>

class MainWindow;
class QDialogButtonBox;

class SettingsPage : public QWidget
{
	Q_OBJECT
public:
	SettingsPage(MainWindow* main_window, QWidget *parent = 0) : QWidget(parent), main_window(main_window){}
	virtual void cancel() = 0;
	virtual void apply();
	virtual void ok(){ this->apply(); }
	virtual QString title() = 0;

protected:
	MainWindow* main_window;

	//The changes to be done when accepted
	QHash<QString, QVariant> changes;
};

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget *parent = 0);

private:
	QTabWidget *tabWidget;
	QDialogButtonBox *dbb;
	QList<SettingsPage*> pages;

private slots:
	void buttonPressed(QAbstractButton*);
};

class RenderPage : public SettingsPage
{
	Q_OBJECT
public:
	RenderPage(MainWindow* main_window, QWidget* parent = 0);

	virtual void cancel(){ changes.clear(); }
	virtual QString title(){ return QString("Map Display"); }
	virtual void apply();
private slots:
	void antialiasingClicked();
	void noantialiasingClicked();
};

#endif
