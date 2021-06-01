#ifndef SHADOWSOCKSWINDOW_H
#define SHADOWSOCKSWINDOW_H

#include <QMainWindow>
#include "PasswordDialog.h"

namespace Ui {
class ShadowSocksWindow;
}

class _ShadowSocksController;

class ShadowSocksWindow : public QMainWindow
{
	Q_OBJECT
	PasswordDialog * dial;

public:
	explicit ShadowSocksWindow(QWidget *parent = 0);
	~ShadowSocksWindow();

public slots:
	void PasswordDialogOk();
	void PasswordDialogFalse();
	//void onCreateServer();

private:
	Ui::ShadowSocksWindow *ui;
	_ShadowSocksController & controler;
};

#endif // SHADOWSOCKSWINDOW_H
