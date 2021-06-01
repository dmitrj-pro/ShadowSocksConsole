#include "PasswordDialog.h"
#include "ui_PasswordDialog.h"

PasswordDialog::PasswordDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PasswordDialog)
{
	ui->setupUi(this);
	setLayout(ui->verticalLayout);
	setError();
}

QString PasswordDialog::getPassword() {
	return ui->lineEdit->text();
}

void PasswordDialog::setError(const QString & text) {
	if (text.size() == 0) {
		ui->label->setText(text);
		ui->label->setVisible(false);
	} else {
		ui->label->setText(text);
		ui->label->setVisible(true);
	}
}

PasswordDialog::~PasswordDialog()
{
	delete ui;
}
