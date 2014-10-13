/**
 * @file networkconfigdialog.cpp
 * This Class defines application Network Configuration dialog window.
 * @brief Source file for NetworkConfigDialog class definition.
 *
 * This dialog window is created through <B>Configuration->Network Config.</B> menu.
 *
 * @author Plinio Andrade &lt;PAndrade@fele.com&gt;
 * @version 1.0.0.0 (Qt: 5.3.1)
 */

#include "networkconfigdialog.h"
#include "ui_networkconfigdialog.h"
#include <QPushButton>
#include <QSettings>
#include "version.h"
//#include <QDebug>
NetworkConfigDialog::NetworkConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetworkConfigDialog)
{
    ui->setupUi(this);
    //Removes 'What's It?' buttom in this dialog window.
    this->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
#ifdef Q_OS_ANDROID
    this->showMaximized();//Show Full Screen for Android Devices
#endif
    this->ui->customAdress_checkBox->setChecked(false);
    this->ui->customAddress_groupBox->setEnabled(false);
    QObject::connect(this->ui->customAdress_checkBox,SIGNAL(toggled(bool)),this,SLOT(checkBoxToggled(bool)));
    QObject::connect(this->ui->customAddresslineEdit, SIGNAL(textChanged(QString)), this, SLOT(addressTextChanged(QString)));
    QObject::connect(this->ui->buttonBox, SIGNAL(accepted()), this, SLOT(saveUserPreferences()));
    loadUerPreferences();
}

NetworkConfigDialog::~NetworkConfigDialog()
{
    delete ui;
}
void NetworkConfigDialog::addressTextChanged(QString text){
    if(this->ui->customAdress_checkBox->isChecked() && text.isEmpty()){
        this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }else{
        this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}
QString NetworkConfigDialog::getAddress(){
    return this->ui->customAddresslineEdit->text();
}
int NetworkConfigDialog::getTimeout(){
    return this->ui->timeout_spinBox->value()*1000;
}
bool NetworkConfigDialog::getcustomAddressCheckBoxState(){
    return this->ui->customAdress_checkBox->isEnabled();
}
void NetworkConfigDialog::saveUserPreferences(){
    QString curText = this->ui->customAddresslineEdit->text();
    if(!curText.contains("http://")){
        this->ui->customAddresslineEdit->setText("http://"+curText);
    }
    QSettings toSave(QSettings::IniFormat,QSettings::UserScope,VER_COMPANYNAME_STR,VER_FILEDESCRIPTION_STR);
    toSave.beginGroup("NetworkConfig");
    toSave.setValue("size",this->size());
    toSave.setValue("pos",this->pos());
    toSave.setValue("customAddressEnabled",this->ui->customAdress_checkBox->isChecked());
    toSave.setValue("customAddress",this->ui->customAddresslineEdit->text());
    toSave.setValue("timeoutValue",this->ui->timeout_spinBox->value());
    toSave.endGroup();

}
void NetworkConfigDialog::loadUerPreferences(){
    QSettings toLoad(QSettings::IniFormat,QSettings::UserScope,VER_COMPANYNAME_STR,VER_FILEDESCRIPTION_STR);
    toLoad.beginGroup("NetworkConfig");
    this->resize(toLoad.value("size", QSize(400,250)).toSize());
    this->move(toLoad.value("pos", QPoint(200, 200)).toPoint());
    this->ui->customAdress_checkBox->setChecked(toLoad.value("customAddressEnabled", false).toBool());
    this->ui->customAddresslineEdit->setText(toLoad.value("customAddress", "http://192.168.240.1:80").toString());
    this->ui->timeout_spinBox->setValue(toLoad.value("timeoutValue", 14).toInt());
    toLoad.endGroup();
}
void NetworkConfigDialog::showEvent(QShowEvent * event){
    loadUerPreferences();
    event->accept();
}
void NetworkConfigDialog::checkBoxToggled(bool checked){
    this->ui->customAddress_groupBox->setEnabled(checked);
    if(checked){
        emit this->ui->customAddresslineEdit->textChanged(this->ui->customAddresslineEdit->text());
    }else{
        this->ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    }
}
