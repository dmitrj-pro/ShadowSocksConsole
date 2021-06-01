#include "ShadowSocksWindow.h"
#include "ui_ShadowSocksWindow.h"
#include "ShadowSocksMain.h"
#include "ShadowSocksController.h"
#include <_Driver/Service.h>
#include <Types/Exception.h>

ShadowSocksWindow::ShadowSocksWindow(QWidget *parent) :
	QMainWindow(parent),
	controler(ShadowSocksController::Get()),
	ui(new Ui::ShadowSocksWindow)
{
	ui->setupUi(this);
	ui->centralwidget->setLayout(ui->verticalLayout);
	ui->tab_2->setLayout(ui->verticalLayout_2);


	if (! controler.isCreated() || (controler.isCreated() && controler.isEncrypted())) {
		dial = new PasswordDialog(this);
		dial->setModal(true);
		dial->show();
		connect(dial, SIGNAL(accepted()), this, SLOT(PasswordDialogOk()));
		connect(dial, SIGNAL(rejected()), this, SLOT(PasswordDialogFalse()));
	}

}

void ShadowSocksWindow::PasswordDialogOk() {
	try {
		if (controler.isCreated()) {
			controler.OpenConfig(dial->getPassword().toStdString());
		} else {
			controler.SetPassword(dial->getPassword().toStdString());
			controler.SaveConfig();
		}
	} catch (__DP_LIB_NAMESPACE__::BaseException e) {
		dial->setError("Password is wrong");
		dial->show();
	}
}
void ShadowSocksWindow::PasswordDialogFalse() {
	if (controler.isCreated()) {
		controler.MakeExit();
		return;
	}
	controler.SaveConfig();
}

ShadowSocksWindow::~ShadowSocksWindow()
{
	delete ui;
}
