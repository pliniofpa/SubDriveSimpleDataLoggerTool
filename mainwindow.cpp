/*! \mainpage %SubDrive V2 Data logger  1.0.0.0 Documentation

\image html logo.png

This is a cross-platform coded application designed for work with Franklin Electric SubDrive V2 motor driver.
The GUI (Graphical User Interface) of this application was created using Qt Creator.
Most of the code of this application is in the main class MainWindow in the mainwindow.cpp file.

In case of questions contact the authors:
<p>Lynn, Doug &lt;<a href="mailto:DLynn@fele.com?Subject=Subdrive%20Data%20Logger">DLynn@fele.com</a>&gt;</p>
<p>Andrade, Plinio &lt;<a href="mailto:PAndrade@fele.com?Subject=Subdrive%20Data%20Logger">PAndrade@fele.com</a>&gt;</p>

\section mainpage-howitworks How does it work?
This application connects to the wireless module in the NepTune Inverter Driver and requests data in the interval difined
in the \code
MainWindow::requestTimer.setInterval(); \endcode
function. The data is then parsed and the General Info tab fields and the tables are populated with it. \n

The data can be exported to a CSV File and the same way the data can be imported from a CSV File. There is the
option of set up a file name and time interval in the <B>Configuration->Auto Save Data</B> in order to allow the application save the
last received data to a especific file each specified time interval.\n
In the <B>Configuration->Multi Drivers Config.</B> menu is possible to change the mode of operation to Network Roaming Mode.
In this mode of operation the application will alternate among selected wireless networks. The application will automatically
exchange among the selected wireless networks every cycle of data./n

\warning The Network Roaming Mode isn't available on Android platform.

\section mainpage-graphics Graphics
\a This application has a built-in graphic library called QCustomPlot. If you want to know more about
  QCustomPlot and just want to start using it, it's recommended to look at the tutorials and
  examples at

  http://www.qcustomplot.com/

\subsection mainpage-graphics-dplot Exporting to DPLot
The Export to DPlot feature exports the current, voltage, and temperature data from General Info table to
DPlot or DPlot Jr software installed on the system. The DPlot Jr is free for download and can be found here:\n
http://www.dplot.com/other.htm
This feature uses the external shared library "DPLOTLIB.DLL" and is available <B>only</B>
on Windows platform. The shared library must to be placed in the same directory of this applicaitan executable file. For more
information about using this library visit the documentation web site:\n
http://www.dplot.com/lib/index.htm?home.htm
\warning The Export to DPlot will not work property if "DPLOTLIB.DLL" isn't present in the same directory of this
application executable file or if the system doesn't have DPlot/DPlot Jr software installed on it.
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTableWidget>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QDialog>
#include <QDomDocument>
#include <QFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QTableWidgetItem>
#include "aboutdialog.h"
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include "version.h"
#include "networkconfigdialog.h"
#include "emailconfig.h"
#include "smtp/smtp.h"
#include <QWindow>
#include <QScroller>
#include <QStandardPaths>
#define ITEMS_LIMIT 100
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->manager=NULL;
    this->reply = NULL;
    this->makeconnections();
    this->ui->loggroupBox->setVisible(false);
    this->logactive = false;
    this->configButtonPressCount = 0;
    this->createtables();
    this->stoptimer = false;
    this->previous_group = -1;
    this->networkConfigDialog = new NetworkConfigDialog(this);
    //Helper variable for limit the number of Logs in the General Info Table.
    this->generalInfoLogNumber = 0;
    this->curSSID = "";
    curNetworkListIndex = 0;
    this->generalinfo_tableWidget = 0;
    requestTimeOut.setSingleShot(true);
    this->customAddress = this->networkConfigDialog->getAddress()+"/gainspan/profile/mcu";
    this->requestTimeOut.setInterval(this->networkConfigDialog->getTimeout());//Set Timeout
    this->loadUerPreferences();
    this->ui->menuConfig->setEnabled(false);
    this->ui->email_pushButton->setDisabled(true);
    //this->fileName = this->fileName = fileDir+"/test.csv";
    QScroller::grabGesture(this->ui->log_textEdit_2->viewport(), QScroller::LeftMouseButtonGesture);    
}

void MainWindow::makeconnections(){
    //Makes menu actions triggered signal connections
    QObject::connect(this->ui->actionAboutSubDrive,SIGNAL(triggered()),this,SLOT(aboutApp()));
    QObject::connect(this->ui->actionNetwork_Config,SIGNAL(triggered()),this,SLOT(networkConfigDialogSlot()));
    //Makes checkable items toogled signal connections
    QObject::connect(this->ui->actionShow_Log_Box,SIGNAL(toggled(bool)),this,SLOT(LogBoxVisible(bool)));
    //Makes Timers timeout signal connections
    QObject::connect(&requestTimer, SIGNAL(timeout()), this, SLOT(request()));
    QObject::connect(&requestTimeOut, SIGNAL(timeout()), this, SLOT(abortRequest()));
    QObject::connect(this->ui->actionEmail,SIGNAL(triggered()),this,SLOT(showEmailConfigDialog()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::networkConfigDialogSlot(){
    if(this->networkConfigDialog->exec()){
        if(this->networkConfigDialog->getcustomAddressCheckBoxState()){
            this->customAddress = this->networkConfigDialog->getAddress()+"/gainspan/profile/mcu";
        }
        this->requestTimeOut.setInterval(this->networkConfigDialog->getTimeout());//Set Timeout
    }
}
void MainWindow::request(){
    if(this->stoptimer){
        this->requestTimer.stop();
        return;
    }
    if(this->requestTimer.isActive()){
        this->requestTimer.disconnect();
    }
    if(this->manager==NULL){
        this->manager = new QNetworkAccessManager(this);
    }
    if(this->logactive){
        this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+tr("Connecting to %1").arg(this->curSSID));
    }
    if(this->reply){
        delete this->reply;
    }
    //qDebug() << Q_FUNC_INFO <<this->customAddress;
    if(this->networkConfigDialog->getcustomAddressCheckBoxState()){
        this->reply = manager->get(QNetworkRequest(QUrl(this->customAddress)));
    }else{
        this->reply = manager->get(QNetworkRequest(QUrl(QString("http://192.168.240.1/gainspan/profile/mcu"))));
    }
    //qDebug() << Q_FUNC_INFO << requestTimeOut.interval();
    requestTimeOut.start();//Start Timeout Timer
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(parseXML()));
    //qDebug() << Q_FUNC_INFO <<"After";
}
void MainWindow::abortRequest(){
    //qDebug() << Q_FUNC_INFO;
    if(this->reply){
        this->reply->abort();
    }
    //Disconnect and Connect requestTimer
    requestTimer.disconnect();
    QObject::connect(&requestTimer, SIGNAL(timeout()), this, SLOT(request()));
    this->requestTimer.start();
}

void MainWindow::aboutApp(){
    AboutDialog about(this);
    about.setVersionText(QString("<html><head/><body><p><span style='font-size:10pt; font-weight:600;'>Version: </span><span style='font-size:10pt;'>%1</span></p></body></html>").arg(QString(VERSION_ABOUT)));
    about.exec();
}

void MainWindow::on_start_pushButton_clicked()
{
    if(this->ui->start_pushButton->text()=="Start")
    {
        if(this->fileName.isEmpty()){
            this->ui->email_pushButton->setEnabled(false);
        }
        if(!this->requestTimer.isActive()){
            requestTimer.setInterval(750);//Request Loop Interval
            requestTimer.start();
            this->stoptimer = false;
        }
        //Change the text
        this->ui->start_pushButton->setText("Stop");
        this->ui->start_pushButton->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 rgba(255, 0, 0, 255),\
                                                  stop:1 rgba(255, 133, 135, 255)); border-radius: 10px; font: bold;");
    }else{
        this->stoptimer = true;
        this->abortRequest();
        //Change the text
        this->ui->start_pushButton->setText("Start");
        this->ui->start_pushButton->setStyleSheet("background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:1, stop:0 rgba(0, 92, 168, 255), \
                                                  stop:1 rgba(85, 170, 255, 255)); border-radius: 10px; font: bold;");
    }
}
void MainWindow::parseXML()
{
    requestTimeOut.stop();
    if(reply->error()==QNetworkReply::NoError){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+tr("Succesful Requisition"));
    }else{
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+tr("Requisition Error %1").arg((int)reply->error()));
        QObject::connect(&requestTimer, SIGNAL(timeout()), this, SLOT(request()));
        return;//Interrupt this function in case of error
    }
    QDomDocument* doc = new QDomDocument();
    QString *error = new QString();
    int *a = new int(-1);
    int *b = new int(-1);
    QByteArray replyByteArray = reply->readAll();
    if (!doc->setContent(replyByteArray,error,a,b)) {
        if(this->logactive){
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+tr("Error: ")+*error+tr(" parsing requisition for group %1.").arg(this->previous_group+1) +tr(" line: %1, column: %2").arg(*a).arg(*b));
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+replyByteArray);
        }
    }else{
        if(this->logactive){
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+doc->toString());
        }
        switch (doc->elementsByTagName("GRUP").at(0).toElement().text().toInt()) {
        case 0:
            if(this->previous_group==-1){
                this->parsegroup0(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("0");
            }
            break;
        case 1:
            if(this->previous_group==0){
                this->parsegroup1(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("1");
            }

            break;
        case 2:
            if(this->previous_group==1){
                this->parsegroup2(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("2");
            }

            break;
        case 3:
            if(this->previous_group==2){
                this->parsegroup3(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("3");
            }

            break;
        case 4:
            if(this->previous_group==3){
                this->parsegroup4(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("4");
            }

            break;
        case 5:
            if(this->previous_group==4){
                this->parsegroup5(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("5");
            }

            break;
        case 6:
            if(this->previous_group==5){
                this->parsegroup6(doc);
                this->previous_group++;
                //this->ui->datagroup_lineEdit->setText("6");
            }
            if(this->previous_group==6){
                qDebug()<<this->generalInfoMap;
                this->UpdateGeneralInfoTable();
                this->UpdateFaultEventText();
            }
            this->previous_group=-1;

            break;
        default:
            break;
        }
    }
    //requestTimer.start();
    QObject::connect(&requestTimer, SIGNAL(timeout()), this, SLOT(request()));
}
//PARSE GROUP0
void MainWindow::parsegroup0(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 0");
        return;
    }
    int tempValue = -1;
    QString *tmpfield;
    //Field DPSW on GR0UP0
    tempValue = doc->elementsByTagName("DPSW").at(0).toElement().text().toInt();
    if(tempValue!=-1){
        //FE Connect Dip Switch
        tmpfield = &this->generalInfoMap["FE Connect Dip Switch"];
        if ((tempValue & 0x01) == 0x01)
        {
            *tmpfield=(tr("ON"));

        }
        else
        {
            *tmpfield=(tr("OFF"));
        }
        //Drive Type Dip Switch
        tmpfield = &this->generalInfoMap["Drive Type Dip Switch"];
        if ((tempValue & 0x02) == 0x02)
        {
            *tmpfield=(tr("MonoDrive"));
        }
        else
        {
            *tmpfield=(tr("SubDrive"));
        }
        //Steady Flow
        tmpfield = &this->generalInfoMap["Steady Flow"];
        *tmpfield=((tempValue & 0x08)?"Enabled":"Disabled");
        tempValue = -1;
    }
    //Field FECN on GR0UP0
    tempValue = doc->elementsByTagName("FECN").at(0).toElement().text().toInt();
    if(tempValue!=-1){
        //HP/kW
        tmpfield = &this->generalInfoMap["HP / kW"];
        if ((tempValue & 0x01) == 0x01)
        {
            *tmpfield=("kW");
        }
        else
        {
            *tmpfield=("hp");
        }
        //Bump
        tmpfield = &this->generalInfoMap["Bump"];
        if ((tempValue & 0x02) == 0x02)
        {
            *tmpfield=(tr("Disabled"));
        }
        else
        {
            *tmpfield=(tr("Enabled"));
        }
        //Agressive Bump
        tmpfield = &this->generalInfoMap["Agressive Bump"];
        if ((tempValue & 0x04) == 0x04)
        {
            *tmpfield=(tr("Enabled"));
        }
        else
        {
            *tmpfield=(tr("Disabled"));
        }
        tmpfield = &this->generalInfoMap["Tank Size"];
        if ((tempValue & 0x08) == 0x08)
        {
            *tmpfield=(tr("Large"));
        }
        else
        {
            *tmpfield=(tr("Small"));
        }
        tmpfield = &this->generalInfoMap["Broken Pipe"];
        if ((tempValue & 0x10) == 0x10)
        {
            *tmpfield=(tr("Disabled"));
        }
        else
        {
            *tmpfield=(tr("Enabled"));
        }
        tmpfield = &this->generalInfoMap["Blurred Carrier"];
        if ((tempValue & 0x20) == 0x20)
        {
            *tmpfield=(tr("Enabled"));
        }
        else
        {
            *tmpfield=(tr("Disabled"));
        }
        tmpfield = &this->generalInfoMap["Constant Minimum Fan"];
        if ((tempValue & 0x40) == 0x40)
        {
            *tmpfield=(tr("Enabled"));
        }
        else
        {
            *tmpfield=(tr("Disabled"));
        }
        //Pot Mude
        tmpfield = &this->generalInfoMap["Pot Mode"];
        *tmpfield=((tempValue & 0x100)?"Enabled":"Disabled");
        tempValue = -1;
    }
    //Field MTSW on GR0UP0
    tmpfield = &this->generalInfoMap["Motor Size"];
    *tmpfield=(doc->elementsByTagName("MTSW").at(0).toElement().text());
    //Field PMSW on GROUP0
    tmpfield = &this->generalInfoMap["Pump Size"];
    *tmpfield=(doc->elementsByTagName("PMSW").at(0).toElement().text());
    //Field USNS on GROUP0
    tmpfield = &this->generalInfoMap["Underload Sense Value"];
    *tmpfield=(doc->elementsByTagName("USNS").at(0).toElement().text());
    //Field UTHR on GROUP0
    tmpfield = &this->generalInfoMap["Underload Hours"];
    *tmpfield=(doc->elementsByTagName("UTHR").at(0).toElement().text());
    //Field UTMN on GROUP0
    tmpfield = &this->generalInfoMap["Underload Minutes"];
    *tmpfield=(doc->elementsByTagName("UTMN").at(0).toElement().text());
    //Field UTSC on GROUP0
    tmpfield = &this->generalInfoMap["Underload Seconds"];
    *tmpfield=(doc->elementsByTagName("UTSC").at(0).toElement().text());
    //Field MXFQ on GROUP0
    tmpfield = &this->generalInfoMap["Maximum Frequency"];
    *tmpfield=(doc->elementsByTagName("MXFQ").at(0).toElement().text());
    //Field MNFQ on GROUP0
    tmpfield = &this->generalInfoMap["Minimum Frenquency"];
    *tmpfield=(doc->elementsByTagName("MNFQ").at(0).toElement().text());
    //Field LANG on GROUP0
    tmpfield = &this->generalInfoMap["Language"];
    *tmpfield=(doc->elementsByTagName("LANG").at(0).toElement().text());
    //Fields PTMD-SN12 on GROUP0
    tmpfield = &this->generalInfoMap["Serial Number"];
    QDomElement tmpele = doc->elementsByTagName("SN01").at(0).toElement();
    QString tmpstr = tmpele.text();
    *tmpfield=(tmpstr);
    for(int i=0;i<12;i++){
        tmpele = tmpele.nextSiblingElement();
        tmpstr+=tmpele.text();
    }
    *tmpfield=(tmpstr);
}
//PARSE GROUP1
void MainWindow::parsegroup1(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 1");
        return;
    }
}
//PARSE GROUP2
void MainWindow::parsegroup2(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 2");
        return;
    }

    double tempValue = -1;
    QString *tmpfield;
    //Field VIN1 on GR0UP2
    tempValue = doc->elementsByTagName("VIN1").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Input Voltage (V rms)"];
        *tmpfield=(QString::number(tempValue/100));
        tempValue = -1;
    }
    //Field VOTA on GR0UP2
    tempValue = doc->elementsByTagName("VOTA").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Voltage A (V rms)"];
        *tmpfield=(QString::number(tempValue/100));
        tempValue = -1;
    }
    //Field VOTB on GR0UP2
    tempValue = doc->elementsByTagName("VOTB").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Voltage B (V rms)"];
        *tmpfield=(QString::number(tempValue/100));
        tempValue = -1;
    }
    //Field VOTC on GR0UP2
    tempValue = doc->elementsByTagName("VOTC").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Voltage C (V rms)"];
        *tmpfield=(QString::number(tempValue/100));
        tempValue = -1;
    }
    //Field IOTA on GR0UP2
    tempValue = doc->elementsByTagName("IOTA").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Current A (A rms)"];
        *tmpfield=(QString::number(tempValue/10));
        tempValue = -1;
    }
    //Field IOTB on GR0UP2
    tempValue = doc->elementsByTagName("IOTB").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Current B (A rms)"];
        *tmpfield=(QString::number(tempValue/10));
        tempValue = -1;
    }
    //Field IOTC on GR0UP2
    tempValue = doc->elementsByTagName("IOTC").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Output Current C (A rms)"];
        *tmpfield=(QString::number(tempValue/10));
        tempValue = -1;
    }
    //Field FOUT on GR0UP2
    tmpfield = &this->generalInfoMap["Output Frenquency (Hz)"];
    *tmpfield=(doc->elementsByTagName("FOUT").at(0).toElement().text());
    //Field DMND on GR0UP2
    tmpfield = &this->generalInfoMap["% Demand"];
    *tmpfield=(doc->elementsByTagName("DMND").at(0).toElement().text());
    //Field PTMP on GR0UP2
    tempValue = doc->elementsByTagName("PTMP").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["PFC Temperature"];
        *tmpfield=(QString::number(tempValue/10));
        tempValue = -1;
    }
    //Field DVST on GR0UP2
    int DriveState = doc->elementsByTagName("DVST").at(0).toElement().text().toInt();
    //Field CTFT on GR0UP2
    int FaultState = doc->elementsByTagName("CTFT").at(0).toElement().text().toInt();
    tmpfield = &this->generalInfoMap["Drive Status"];
    if (FaultState > 0)
    {
        *tmpfield=(tr("Fault ") + QString::number(FaultState));
    }
    else if (DriveState == 0)
    {
        *tmpfield=(tr("Drive Not Running"));
    }
    else if (DriveState == 1)
    {
        *tmpfield=(tr("Drive Running"));
    }
    //Field ITMP on GR0UP2
    tempValue = doc->elementsByTagName("ITMP").at(0).toElement().text().toFloat();
    if(tempValue!=-1){
        tmpfield = &this->generalInfoMap["Inverter Temperature"];
        *tmpfield=(QString::number(tempValue/10));
        tempValue = -1;
    }
}
void MainWindow::parsegroup3(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 3");
        return;
    }
    QString *tmpfield;
    //Field MBS0 on GR0UP3
    tmpfield = &this->generalInfoMap["MCB S/W Version"];
    QString aux;
    aux=(doc->elementsByTagName("MBS0").at(0).toElement().text());
    //Field MBS1 on GR0UP3
    //tmpfield = &this->generalInfoMap["MCB S/W Version"];
    *tmpfield=(aux + doc->elementsByTagName("MBS1").at(0).toElement().text());
    //Field DBS0 on GR0UP3
    tmpfield = &this->generalInfoMap["DWB S/W Version"];
    aux=(doc->elementsByTagName("DBS0").at(0).toElement().text());
    //Field DBS1 on GR0UP3
    //tmpfield = &this->generalInfoMap["DWB S/W Version"];
    *tmpfield=(aux + doc->elementsByTagName("DBS1").at(0).toElement().text());
    //Field PKID on GR0UP3
    tmpfield = &this->generalInfoMap["Package ID"];
    *tmpfield=(doc->elementsByTagName("PKID").at(0).toElement().text());
    //Field MDNR on GR0UP3
    tmpfield = &this->generalInfoMap["Model Number"];
    *tmpfield=(doc->elementsByTagName("MDNR").at(0).toElement().text());
    //Field HWVR on GR0UP3
    tmpfield = &this->generalInfoMap["Hardware Version"];
    *tmpfield=(doc->elementsByTagName("HWVR").at(0).toElement().text());


}
void MainWindow::parsegroup4(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 4");
        return;
    }
    //Fields VL01-VL31 on GROUP4
    QDomElement tmpele = doc->elementsByTagName("VL01").at(0).toElement();
    for(int i=0;i<31;i++){
        this->FaultEventInfo[i] = tmpele.text().toInt();
        tmpele = tmpele.nextSiblingElement();
    }
}
void MainWindow::parsegroup5(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 5");
        return;
    }
    //Fields VL01-VL14 on GROUP5
    /*
    QDomElement tmpele = doc->elementsByTagName("VL01").at(0).toElement();
    for(int i=14;i<28;i++){
        this->FaultEventInfo[i] = tmpele.text().toInt();
        tmpele = tmpele.nextSiblingElement();
    }*/
}
void MainWindow::parsegroup6(QDomDocument *doc){
    if(!doc){
        if(this->logactive)
            this->ui->log_textEdit_2->append("["+QTime::currentTime().toString()+"]: "+"Eror parsing Group 6");
        return;
    }
    //Define the Type of Log
    FaultEventInfo[31] = doc->elementsByTagName("VL04").at(0).toElement().text().toInt();
    //Define the Number of Log
    FaultEventInfo[32] = doc->elementsByTagName("VL05").at(0).toElement().text().toInt();
    //Fields VL01-VL06 on GROUP6
    /*
    QDomElement tmpele = doc->elementsByTagName("VL01").at(0).toElement();
    for(int i=28;i<34;i++){
        this->FaultEventInfo[i] = tmpele.text().toInt();
        tmpele = tmpele.nextSiblingElement();
    }*/
}


void MainWindow::on_clearlog_pushButton_clicked()
{
    this->ui->log_textEdit_2->clear();
    //qDebug()<<"Logical DPI "<<QPaintDevice::logicalDpiX()<<"x"<<QPaintDevice::logicalDpiY();
    //qDebug()<<"Physical DPI"<<QPaintDevice::physicalDpiX()<<"x"<<QPaintDevice::physicalDpiY();
    //qDebug()<<"Device Pixel Ratio"<<this->devicePixelRatio();
}

void MainWindow::on_log_pushButton_2_clicked()
{
    if(this->logactive){
        this->ui->log_pushButton_2->setText(tr("Start Log"));
        this->logactive = false;
    }
    else{
        this->ui->log_pushButton_2->setText(tr("Stop Log"));
        this->logactive=true;
    }
}
void MainWindow::UpdateFaultEventText(){
    switch (FaultEventInfo[31])
    {
    case 0:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToFaultTable()";
        //store fault log data
        ConvertReceivedArrayDataToFaultTable();
        break;
    case 1:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToConfigChangeEventTable()";
        //store configuration log data
        ConvertReceivedArrayDataToConfigChangeEventTable();
        break;
    case 2:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToResetLogTable()";
        //store reset log data
        ConvertReceivedArrayDataToResetLogTable();
        break;
    case 3:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToTemperatureLimitTable()";
        //store temperature limit/foldback event data
        ConvertReceivedArrayDataToTemperatureLimitTable();
        break;
    case 4:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToMotorOnEventTable()";
        //store motor on time log data
        ConvertReceivedArrayDataToMotorOnEventTable();
        break;
    case 5:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToPowerOnEventTable()";
        //store power on time log data
        ConvertReceivedArrayDataToPowerOnEventTable();
        break;
    case 6:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToCommunicationsErrorLogTable()";
        //store communications error log data
        ConvertReceivedArrayDataToCommunicationsErrorLogTable();
        break;
    case 7:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToCurrentLimitTable()";
        //store temperature limit/foldback event data
        ConvertReceivedArrayDataToCurrentLimitTable();
        break;
    case 8:
        //qDebug() << Q_FUNC_INFO << "ConvertReceivedArrayDataToTestFixtureDataStoreLogTable()";
        //store text fixture data store data
        ConvertReceivedArrayDataToTestFixtureDataStoreLogTable();
        break;
    }
}
void MainWindow::ConvertReceivedArrayDataToFaultTable(){
    QTableWidget *curTable = this->faulthistory_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //curTable->setRowCount(curTable->rowCount()+1);
    QTableWidgetItem *newItem;
    //Insert Item in the Log # Column
    //newItem = new QTableWidgetItem((curTable->rowCount()));
    newItem = new QTableWidgetItem(QString::number(logNumber));
    being_added<<newItem->text();
    //being_added<<newItem->data(Qt::EditRole).toString();
    QFont tmpfont = newItem->font();
    tmpfont.setBold(true);
    newItem->setFont(tmpfont);
    curTable->setItem(curRow,0,newItem);
    //on time total
    int ontimetotal = FaultEventInfo[5] * 65536 + FaultEventInfo[4];
    //Insert Item in the Days Column
    newItem = new QTableWidgetItem(QString::number(ontimetotal/60/60/24));
    being_added<<newItem->text();
    curTable->setItem(curRow,1,newItem);
    //Insert Item in the Hours Column
    newItem = new QTableWidgetItem(QString::number((ontimetotal%24)/60/60));
    being_added<<newItem->text();
    curTable->setItem(curRow,2,newItem);
    //Insert Item in the Minutes Column
    newItem = new QTableWidgetItem(QString::number((ontimetotal% (24*60)) / 60));
    being_added<<newItem->text();
    curTable->setItem(curRow,3,newItem);
    //Insert Item in the Fault Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[0]%256));
    being_added<<newItem->text();
    curTable->setItem(curRow,4,newItem);
    //Insert Item in the Fault Counter Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[0]/256));
    being_added<<newItem->text();
    curTable->setItem(curRow,5,newItem);
    //Insert Item in the Volt. Bus Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[6]/100));
    being_added<<newItem->text();
    curTable->setItem(curRow,6,newItem);
    //Insert Item in the Volt. Rect Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[7]/100));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);
    //Insert Item in the I(A) Phase A Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[8]/10));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);
    //Insert Item in the I(A) Phase B Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[9]/10));
    being_added<<newItem->text();
    curTable->setItem(curRow,9,newItem);
    //Insert Item in the I(A) Phase C Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[10]/10));
    being_added<<newItem->text();
    curTable->setItem(curRow,10,newItem);
    //Insert Item in the MCB SW Column
    QString tmp_string("");
    tmp_string.append(QString::number(this->FaultEventInfo[3] & 0xFF00).append("."));
    tmp_string.append(QString::number(this->FaultEventInfo[3] & 0x00FF).append("."));
    tmp_string.append(QString::number((this->FaultEventInfo[2] & 0xFF00)>>8).append("."));
    tmp_string.append(QString((char)this->FaultEventInfo[2] & 0x00FF));
    newItem = new QTableWidgetItem(tmp_string);
    being_added<<newItem->text();
    curTable->setItem(curRow,11,newItem);
    //Insert Item in the Max Speed Allowed Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[14]));
    being_added<<newItem->text();
    curTable->setItem(curRow,12,newItem);
    //Insert Item in the Targed Speed Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[15]));
    being_added<<newItem->text();
    curTable->setItem(curRow,13,newItem);
    //Insert Item in the Speed Limiter/Motor ID Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[16]));
    being_added<<newItem->text();
    curTable->setItem(curRow,14,newItem);
    //Insert Item in the Inverter Temperature Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[17]));
    being_added<<newItem->text();
    curTable->setItem(curRow,15,newItem);
    //Insert Item in the PFC Temperature Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[18]));
    being_added<<newItem->text();
    curTable->setItem(curRow,16,newItem);
    //Insert Item in the Fan Speed Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[19]));
    being_added<<newItem->text();
    curTable->setItem(curRow,17,newItem);
    //Insert Item in the Underload Sensitivity Setting Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[20]));
    being_added<<newItem->text();
    curTable->setItem(curRow,18,newItem);
    //Parses Flags and populate columns
    //Insert Item in the Soft Charge Column
    newItem = new QTableWidgetItem(FaultEventInfo[22]&0x0001?"ON":"OFF");
    being_added<<newItem->text();
    curTable->setItem(curRow,19,newItem);
    //Insert Item in the PFC Status Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0002?"ON":"OFF");
    being_added<<newItem->text();
    curTable->setItem(curRow,20,newItem);
    //Insert Item in the Commun. Status Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0004?"Falt":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,21,newItem);
    //Insert Item in the Inverter S.C. Status Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0008?"Fault":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,22,newItem);
    //Insert Item in the Inverter Enabled Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0010?"ON":"OFF");
    being_added<<newItem->text();
    curTable->setItem(curRow,23,newItem);
    //Insert Item in the Bootstrap in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0020?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,24,newItem);
    //Insert Item in the DC Test in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0040?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,25,newItem);
    //Insert Item in the AC Test in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0080?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,26,newItem);
    //Insert Item in the Bump in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0100?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,27,newItem);
    //Insert Item in the AC Pressure in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0200?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,28,newItem);
    //Insert Item in the Shake Motor in Progress Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0400?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,29,newItem);
    //Insert Item in the Open Circuit Detect Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x0800?"Fault":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,30,newItem);
    //Insert Item in the Short Circuit Detect Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x1000?"Fault":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,31,newItem);
    //Insert Item in the Phase Imbalance Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x2000?"Fault":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,32,newItem);
    //Insert Item in the Pressure Switch Closed Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x4000?"Yes":"No");
    being_added<<newItem->text();
    curTable->setItem(curRow,33,newItem);
    //Insert Item in the Hobb Circuit Fault Detect Column
    newItem = new QTableWidgetItem( FaultEventInfo[22]&0x8000?"Fault":"No Fault");
    being_added<<newItem->text();
    curTable->setItem(curRow,34,newItem);
    //Insert Item in the Status Flags (1st two bytes) Column
    int statusflag = FaultEventInfo[21];
    newItem = new QTableWidgetItem(QString::number(statusflag));
    being_added<<newItem->text();
    curTable->setItem(curRow,35,newItem);
    //Insert Item in the Error Exit ID Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[23]));
    being_added<<newItem->text();
    curTable->setItem(curRow,36,newItem);
    //Insert Item in the Current Demand Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[24]));
    being_added<<newItem->text();
    curTable->setItem(curRow,37,newItem);
    //Insert Item in the I2C Status Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[25]));
    being_added<<newItem->text();
    curTable->setItem(curRow,38,newItem);
    //Insert Item in the SSID Column
    newItem = new QTableWidgetItem(this->curSSID);
    being_added<<newItem->text();
    curTable->setItem(curRow,curTable->columnCount()-2,newItem);
    //Insert Item in the Log Date-Time Column
    newItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("dd MMMM yyyy hh:mm:ss"));
    curTable->setItem(curRow,curTable->columnCount()-1,newItem);
    this->ui->statusbar->showMessage(QString("%1: Fault Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}

void MainWindow::ConvertReceivedArrayDataToConfigChangeEventTable(){
    QTableWidget *curTable = this->configevent_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //Insert Item in the FE Connect Configuration Setting Column
    QString tempvar = (this->FaultEventInfo[4]& 0x01)==0?"ON":"OFF";
    newItem = new QTableWidgetItem(tempvar);
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);

    //Insert Item in the Drive Type Column
    tempvar = (this->FaultEventInfo[4]& 0x02)==0?"SubDrive":"MonoDrive";
    newItem = new QTableWidgetItem(tempvar);
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);

    //Insert Item in the Steady Flow Column
    tempvar = (this->FaultEventInfo[4]& 0x08)==0?"Enabled":"Disabled";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,9,newItem);

    //Insert Item in the Dev Config. Column
    int tempvar2 = (this->FaultEventInfo[4]& 0x0C);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,10,newItem);

    //Insert Item in the Motor Size Column
    switch (this->FaultEventInfo[5]) {
    case 1:
        tempvar = "0.5 hp";
        break;
    case 2:
        tempvar = "0.75 hp";
        break;
    case 3:
        tempvar = "1.0 hp";
        break;
    case 4:
        tempvar = "1.5 hp";
        break;
    case 5:
        tempvar = "2.0 hp";
        break;
    case 6:
        tempvar = "3.0 hp";
        break;
    default:
        break;
    }
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,11,newItem);

    //Insert Item in the Pump Size Column
    switch (this->FaultEventInfo[6]) {
    case 1:
        tempvar = "0.5 hp";
        break;
    case 2:
        tempvar = "0.75 hp";
        break;
    case 3:
        tempvar = "1.0 hp";
        break;
    case 4:
        tempvar = "1.5 hp";
        break;
    case 5:
        tempvar = "2.0 hp";
        break;
    case 6:
        tempvar = "3.0 hp";
        break;
    default:
        break;
    }
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,12,newItem);

    //Insert Item in the Underload Setpoint Column
    tempvar2 = (this->FaultEventInfo[7]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,20,newItem);

    //Insert Item in the Units Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x01)==0?"hp":"kW";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,13,newItem);

    //Insert Item in the Bump Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x02)==0?"ON":"OFF";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,14,newItem);

    //Insert Item in the Agressive Bump Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x04)==0?"OFF":"ON";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,15,newItem);

    //Insert Item in the Tank Size Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x08)==0?"Small":"Large";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,16,newItem);

    //Insert Item in the Broken Pipe Disable Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x10)==0?"ON":"OFF";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,17,newItem);

    //Insert Item in the Blurred Carrier Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x20)==0?"ON":"OFF";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,18,newItem);

    //Insert Item in the Minimum Fan Enable Configuration Setting Column
    tempvar = (this->FaultEventInfo[8]& 0x40)==0?"OFF":"ON";
    newItem = new QTableWidgetItem(tempvar);
    curTable->setItem(curRow,19,newItem);

    //Insert Item in the Under Load Hours Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[9]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,21,newItem);

    //Insert Item in the Under Load Minutes Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[10]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,22,newItem);

    //Insert Item in the Under Load Seconds Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[11]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,23,newItem);

    //Insert Item in the Max Frequency Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[12]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,24,newItem);

    //Insert Item in the Min Frequency Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[13]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,25,newItem);

    //Insert Item in the Language Configuration Setting Column
    tempvar2 = (this->FaultEventInfo[14]);
    newItem = new QTableWidgetItem(tempvar2);
    curTable->setItem(curRow,26,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();
    this->ui->statusbar->showMessage(QString("%1: Config Change Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToResetLogTable(){
    QTableWidget *curTable = this->reset_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //Insert Item in the ResetSource Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[4]));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);

    //Insert Item in the ResetType Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[5]));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Reset Log Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));

    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToTemperatureLimitTable(){
    QTableWidget *curTable = this->temp_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //number of events total
    int eventstotal = FaultEventInfo[5] * 65536 + FaultEventInfo[4];
    //Insert Item in the Events Total Column
    newItem = new QTableWidgetItem(QString::number(eventstotal));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);
    //number of events this power cycle
    int eventspowercycle = FaultEventInfo[7] * 65536 + FaultEventInfo[6];
    //Insert Item in the Events Total Column
    newItem = new QTableWidgetItem(QString::number(eventspowercycle));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);
    //start time first event
    int stfirstevent = FaultEventInfo[9] * 65536 + FaultEventInfo[8];
    //Insert Item in the 1st Event Start Time Column
    newItem = new QTableWidgetItem(QString::number(stfirstevent));
    being_added<<newItem->text();
    curTable->setItem(curRow,9,newItem);
    //Insert Item in the Event Source Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[10]));
    being_added<<newItem->text();
    curTable->setItem(curRow,10,newItem);
    //Insert Item in the Inverter Temperature Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[11]));
    being_added<<newItem->text();
    curTable->setItem(curRow,11,newItem);
    //Insert Item in the PFC Temperature Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[12]));
    being_added<<newItem->text();
    curTable->setItem(curRow,12,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Temperature Limit Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToMotorOnEventTable(){
    QTableWidget *curTable = this->motor_ontime_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //motor starts total
    int motorstartstotal = FaultEventInfo[5] * 65536 + FaultEventInfo[4];
    //Insert Item in the Motor Starts Total Column
    newItem = new QTableWidgetItem(QString::number(motorstartstotal));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);
    //motor starts this power cycle
    int motorstartpowercycle = FaultEventInfo[7] * 65536 + FaultEventInfo[6];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(motorstartpowercycle));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Motor Event Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToPowerOnEventTable(){
    QTableWidget *curTable = this->poweron_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Power On Event Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToCommunicationsErrorLogTable(){
    QTableWidget *curTable = this->communication_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //communication errors total
    int commerrorstotal = FaultEventInfo[5] * 65536 + FaultEventInfo[4];
    //Insert Item in the Communication Errors Total Column
    newItem = new QTableWidgetItem(QString::number(commerrorstotal));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);

    //communication errors at power up
    int commerrorsatpowerup = FaultEventInfo[7] * 65536 + FaultEventInfo[6];
    //Insert Item in the Communication Errors at Power Up Column
    newItem = new QTableWidgetItem(QString::number(commerrorsatpowerup));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);

    //communication errors this power cycle
    int commerrorsthispowercycle = FaultEventInfo[9] * 65536 + FaultEventInfo[8];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorsthispowercycle));
    being_added<<newItem->text();
    curTable->setItem(curRow,9,newItem);

    //communication errors total EXB
    int commerrorstotalexb = FaultEventInfo[11] * 65536 + FaultEventInfo[10];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorstotalexb));
    being_added<<newItem->text();
    curTable->setItem(curRow,10,newItem);

    //communication errors at power up EXB
    int commerrorsatpowerupexb = FaultEventInfo[13] * 65536 + FaultEventInfo[12];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorsatpowerupexb));
    being_added<<newItem->text();
    curTable->setItem(curRow,11,newItem);

    //communication errors this power cycle EXB
    int commerrorsthispowercycleexb = FaultEventInfo[15] * 65536 + FaultEventInfo[14];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorsthispowercycleexb));
    //being_added<<newItem->text();
    curTable->setItem(curRow,12,newItem);

    //communication errors total MCB
    int commerrorstotalmcb = FaultEventInfo[17] * 65536 + FaultEventInfo[16];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorstotalmcb));
    being_added<<newItem->text();
    curTable->setItem(curRow,13,newItem);

    //communication errors at power up MCB
    int commerrorsatpowerupmcb = FaultEventInfo[19] * 65536 + FaultEventInfo[18];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorsatpowerupmcb));
    being_added<<newItem->text();
    curTable->setItem(curRow,14,newItem);

    //communication errors this power cycle MCB
    int commerrorsthispowercyclemcb = FaultEventInfo[21] * 65536 + FaultEventInfo[20];
    //Insert Item in the Starts this Power Cycle Column
    newItem = new QTableWidgetItem(QString::number(commerrorsthispowercyclemcb));
    being_added<<newItem->text();
    curTable->setItem(curRow,15,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Communication Error Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToCurrentLimitTable(){

    QTableWidget *curTable = this->current_limit_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,true,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //number of events total
    int eventstotal = FaultEventInfo[5] * 65536 + FaultEventInfo[4];
    //Insert Item in the Events Total Column
    newItem = new QTableWidgetItem(QString::number(eventstotal));
    being_added<<newItem->text();
    curTable->setItem(curRow,7,newItem);
    //number of events this power cycle
    int eventspowercycle = FaultEventInfo[7] * 65536 + FaultEventInfo[6];
    //Insert Item in the Events Total Column
    newItem = new QTableWidgetItem(QString::number(eventspowercycle));
    being_added<<newItem->text();
    curTable->setItem(curRow,8,newItem);
    //start time first event
    int stfirstevent = FaultEventInfo[9] * 65536 + FaultEventInfo[8];
    //Insert Item in the 1st Event Start Time Column
    newItem = new QTableWidgetItem(QString::number(stfirstevent));
    being_added<<newItem->text();
    curTable->setItem(curRow,9,newItem);
    //Insert Item in the Current Phase A Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[10]));
    being_added<<newItem->text();
    curTable->setItem(curRow,10,newItem);
    //Insert Item in the Current Phase B Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[11]));
    being_added<<newItem->text();
    curTable->setItem(curRow,11,newItem);
    //Insert Item in the Current Phase C Column
    newItem = new QTableWidgetItem(QString::number(this->FaultEventInfo[12]));
    being_added<<newItem->text();
    curTable->setItem(curRow,12,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Current Limit Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}
void MainWindow::ConvertReceivedArrayDataToTestFixtureDataStoreLogTable(){

}
void MainWindow::ConvertReceivedArrayDataToOverloadFixtureTable(){

    QTableWidget *curTable = this->overload_event_hist_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    int logNumber = this->FaultEventInfo[32];//That is the Log Number
    QStringList record,being_added;
    int curRow = this->isthereSameLog(curTable,logNumber,&record);
    //Complete Columns Log #, Days, Hours, Mins. for Total On Time and Total On Cycle (if the second parameter is true)
    CompleteDateonTable(curTable,false,logNumber,curRow,&being_added);
    QTableWidgetItem *newItem;
    //Total Number of Events
    int totalnumberofevents = FaultEventInfo[3] * 65536 + FaultEventInfo[2];
    //Insert Item in the Total Number of Events Column
    newItem = new QTableWidgetItem(QString::number(totalnumberofevents));
    being_added<<newItem->text();
    curTable->setItem(curRow,4,newItem);

    //Add SSID to the correct position on being_added StringList
    being_added<<curTable->item(curRow,curTable->columnCount()-2)->text();

    this->ui->statusbar->showMessage(QString("%1: Overload Fixture Record #%2 Captured.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(logNumber));
    //qDebug()<<record;
    //qDebug()<<being_added;
    if(record==being_added){
        this->exportData();
    }
}

void MainWindow::createtables(){
    //Create Fault Table
    this->faulthistory_tableWidget = new QTableWidget();
    QTableWidget *curTable = this->faulthistory_tableWidget;
    curTable->setObjectName("faulthistory_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(41);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Days,Hours,Minutes,Fault,Fault Counter,Volt. Bus,Volt. Rect,I(A) Phase A,I(A) Phase B,I(A) Phase C,MCB SW,Max Speed Allowed,Target Speed,Speed Limiter/Motor ID,Inverter Temp.,PFC Temp.,Fan Speed,Under. Sens. Setting,Soft Charge,PFC Status,Commun. Status,Inverter S.C. Status,Inverter Enabled,Boostrap in Progress,DC Test in Progress,AC Test in Progress,Bump in Progress,AC Pressure in Progress,Shake Motor in Progress,Open Circuit Detect,Short Circuit Detect,Phase Imbalance,Pressure Switch Closed,Hobb Circuit Fault Detect,Status Flags(1st two bytes),Error Exit ID,Current Demand,I2C Status,SSID,Log Date-Time")).split(",")));
    curTable = 0;
    //Create Config. Event Table
    this->configevent_tableWidget = new QTableWidget();
    curTable = this->configevent_tableWidget;
    curTable->setObjectName("configevent_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(29);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,FE Connect,Drive Type,Steady Flow,Dev Config.,Motor Size,Pump Size,Units Config.,Bump Config.,Agress. Bump,Tank Size,Broken Pipe,Blurred Carrier,Min. Fan,Underload Setpoint,Under. Hours,Under. Mins,Under. Secs,Freq. Max,Freq. Min,Language,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Reset Event History Table
    this->reset_event_hist_tableWidget = new QTableWidget();
    curTable = this->reset_event_hist_tableWidget;
    curTable->setObjectName("reset_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(11);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,Reset Source,Reset Type,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Temperature Event History Table
    this->temp_event_hist_tableWidget = new QTableWidget();
    curTable = this->temp_event_hist_tableWidget;
    curTable->setObjectName("temp_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(15);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,Total Events,On this Cycle,1st Event Start Time,Temp. Source,Inverter Temp.,PFC Temp.,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Motor On time Event History Table
    this->motor_ontime_event_hist_tableWidget = new QTableWidget();
    curTable = this->motor_ontime_event_hist_tableWidget;
    curTable->setObjectName("motor_ontime_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(11);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,Total Starts,Starts this Cycle,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Power On Event History Table
    this->poweron_event_hist_tableWidget = new QTableWidget();
    curTable = this->poweron_event_hist_tableWidget;
    curTable->setObjectName("poweron_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(9);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Communication Event History Table
    this->communication_event_hist_tableWidget = new QTableWidget();
    curTable = this->communication_event_hist_tableWidget;
    curTable->setObjectName("communication_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(18);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,DWB Total Errors,DWB Errors at PowerUp,DWB Errors this Cycle,EXB Total Errors,EXB Errors at PowerUp,EXB Errors this Cycle,MCB Total Errors,MCB Errors at PowerUp,MCB Errors this Cycle,SSID,Log Date-Time")).split(",")));
    curTable = 0;

    //Create Current Limit Event History Table
    this->current_limit_event_hist_tableWidget = new QTableWidget();
    curTable = this->current_limit_event_hist_tableWidget;
    curTable->setObjectName("current_limit_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(15);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,Total Events,This Cycle Events,1st Event Start Time,Current Phase A,Current Phase B,Current Phase C,SSID,Log Date-Time")).split(",")));
    curTable = 0;
    /*
    //Create Test Fixture Event History Table
    curTable = this->ui->test_fixture_limit_event_hist_tableWidget;
    curTable->setColumnCount(15);
    //curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,This Cycle On Days,This Cycle On Hours,This Cycle On Mins,Total Events,This Cycle Events,1st Event Start Time,Current Phase A,Current Phase B,Current Phase C,SSID,Log Date-Time")).split(",")));
    //QScroller::grabGesture(curTable->viewport(), QScroller::LeftMouseButtonGesture);
    //Resize Table Columns and Rows
    //curTable->resizeColumnsToContents();
    //curTable->resizeRowsToContents();
    //Set Headers Movable
    //curTable->horizontalHeader()->setSectionsMovable(true);
    curTable = 0;
*/
    //Create Overload Event History Table
    this->overload_event_hist_tableWidget = new QTableWidget();
    curTable = this->overload_event_hist_tableWidget;
    curTable->setObjectName("overload_event_hist_tableWidget");
    this->tables<<curTable;
    curTable->setColumnCount(7);
    curTable->setHorizontalHeaderLabels(QStringList(QString(tr("Log #,Total On Days,Total On Hours,Total On Minutes,Total Events,SSID,Log Date-Time")).split(",")));
    curTable = 0;
}

void MainWindow::UpdateGeneralInfoTable(){
    QTableWidget *curTable;
    if(this->generalinfo_tableWidget==0){
        //Create General Info Table
        this->generalinfo_tableWidget = new QTableWidget();
        curTable = this->generalinfo_tableWidget;
        curTable->setObjectName("generalinfo_tableWidget");
        this->tables<<curTable;
        QStringList h_header;
        h_header << "Request #";
        h_header << this->generalInfoMap.keys();
        h_header <<"SSID";
        h_header << "Log Date-Time";
        curTable->setColumnCount(h_header.length());
        curTable->setHorizontalHeaderLabels(h_header);
        //qDebug()<<h_header;
        //qDebug()<<curTable->horizontalHeader();
        curTable = 0;
    }
    curTable = this->generalinfo_tableWidget;
    //Stop sorting before insert more items
    curTable->setSortingEnabled(false);
    //Define a limit of items in this table
    int curIndex;
    if(curTable->rowCount()>=ITEMS_LIMIT){
        curIndex = this->generalInfoLogNumber%(int)ITEMS_LIMIT;
    }else{
        //Add a new Row
        curTable->setRowCount(curTable->rowCount()+1);
        curIndex = curTable->rowCount()-1;
    }

    QTableWidgetItem *newItem;
    //Insert Item in the Log # Column
    newItem = new QTableWidgetItem(QString::number(this->generalInfoLogNumber+1));
    QFont tmpfont = newItem->font();
    tmpfont.setBold(true);
    newItem->setFont(tmpfont);
    curTable->setItem(curIndex,0,newItem);
    //qDebug()<<QString("Column #: %1, Header: %2 => %3").arg(0).arg(curTable->horizontalHeaderItem(0)->text()).arg(newItem->text());

    //Add items dynamically
    int length = this->generalInfoMap.keys().length()+1;
    for(int i=1;i<length;i++){
        newItem = new QTableWidgetItem(this->generalInfoMap[curTable->horizontalHeaderItem(i)->text()]);
        curTable->setItem(curIndex,i,newItem);
        //qDebug()<<QString("Column #: %1, Header: %2 => %3").arg(i).arg(curTable->horizontalHeaderItem(i)->text()).arg(newItem->text());
    }
    //Insert SSID Column (Last-1 Column)
    newItem = new QTableWidgetItem(this->curSSID);
    curTable->setItem(curIndex,curTable->columnCount()-2,newItem);
    //qDebug()<<QString("Column #: %1, Header: %2 Value: %3").arg(curTable->columnCount()-2).arg(curTable->horizontalHeaderItem(curTable->columnCount()-2)->text()).arg(newItem->text());
    //Insert Item in the Log Date-Time Column (Last Column)
    QDateTime datetime = QDateTime::currentDateTime();
    newItem = new QTableWidgetItem(datetime.toString("dd MMMM yyyy hh:mm:ss"));
    curTable->setItem(curIndex,curTable->columnCount()-1,newItem);
    //qDebug()<<QString("Column #: %1, Header: %2 => %3").arg(curTable->columnCount()-1).arg(curTable->horizontalHeaderItem(curTable->columnCount()-1)->text()).arg(newItem->text());
    //Increment Items number
    this->generalInfoLogNumber++;
}

//This function convert data to date (Days, Hours, Minutes)
QList<int> MainWindow::ConvertTimefromVector(){
    QList<int> data;
    //on time total
    int ontimetotal = FaultEventInfo[1] * 65536 + FaultEventInfo[0];
    //on time this power cycle
    int ontimethiscycle = FaultEventInfo[3] * 65536 + FaultEventInfo[2];
    //Calculate Total on Time Days
    data.append((ontimetotal/60/60/24));
    //Calculate Total on Time Hours
    data.append(((ontimetotal%24)/60/60));
    //Calculate Total on Time Minutes
    data.append(((ontimetotal% (24*60)) / 60));
    //Calculate This Cycle On Time Days
    data.append((ontimethiscycle/60/60/24));
    //Calculate This Cycle On Time Hours
    data.append(((ontimethiscycle%24)/60/60));
    //Calculate This Cycle On Time Minutes
    data.append(((ontimethiscycle% (24*60)) / 60));
    return data;
}
//This function complete commom data on table curTable
void MainWindow::CompleteDateonTable(QTableWidget *curTable, bool thisCycle, int logNumber, int curRow,QStringList *being_added){
    QTableWidgetItem *newItem;
    //Insert Item in the Log # Column
    newItem = new QTableWidgetItem(QString::number(logNumber));
    *being_added<<newItem->text();
    QFont tmpfont = newItem->font();
    tmpfont.setBold(true);
    newItem->setFont(tmpfont);
    curTable->setItem(curRow,0,newItem);
    //Insert Item in the SSID Column
    newItem = new QTableWidgetItem(this->curSSID);
    curTable->setItem(curRow,curTable->columnCount()-2,newItem);
    //Insert Item in the Total On Time Days Column
    newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(0)));
    *being_added<<newItem->text();
    curTable->setItem(curRow,1,newItem);

    //Insert Item in the Total On Time Hours Column
    newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(1)));
    *being_added<<newItem->text();
    curTable->setItem(curRow,2,newItem);

    //Insert Item in the Total On Time Minutes Column
    newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(2)));
    *being_added<<newItem->text();
    curTable->setItem(curRow,3,newItem);
    if(thisCycle){
        //Insert Item in the this Cycle On Time Days Column
        newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(0)));
        *being_added<<newItem->text();
        curTable->setItem(curRow,4,newItem);

        //Insert Item in the this Cycle On Time Hours Column
        newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(1)));
        *being_added<<newItem->text();
        curTable->setItem(curRow,5,newItem);

        //Insert Item in the this Cycle On Time Minutes Column
        newItem = new QTableWidgetItem(QString::number(this->ConvertTimefromVector().at(2)));
        *being_added<<newItem->text();
        curTable->setItem(curRow,6,newItem);
    }
    //Insert Item in the Log Date-Time Column (Last Column)
    newItem = new QTableWidgetItem(QDateTime::currentDateTime().toString("dd MMMM yyyy hh:mm:ss"));
    curTable->setItem(curRow,curTable->columnCount()-1,newItem);

}
//Set LogBox Visible and handle the buttons
void MainWindow::LogBoxVisible(bool visible){
    if(visible){
        this->logactive = true;
        this->ui->log_pushButton_2->setText("Stop Log");
        this->ui->loggroupBox->setVisible(true);
    }else{
        this->logactive=false;
        this->ui->log_pushButton_2->setText("Start Log");
        this->ui->clearlog_pushButton->click();
        this->ui->loggroupBox->setVisible(false);
    }
}
//This function searches for a row with logNumber on curTable and return its row number
//In case of it didn't find the log it will create a new row and return the new row number
//record is output parameter and it contains the current record in the table
int MainWindow::isthereSameLog(QTableWidget *curTable, int logNumber, QStringList *record){
    for(int i=0;i<curTable->rowCount();i++){
        if(curTable->item(i,0)->text()==QString::number(logNumber)){
            if(record!=0){
                for(int j=0;j<curTable->columnCount()-1;++j){
                    *record<<curTable->item(i,j)->text();
                }
            }
            return i;
        }
    }
    curTable->setRowCount(curTable->rowCount()+1);
    return (curTable->rowCount()-1);
}

void MainWindow::on_config_pushButton_clicked()
{
    if(this->configButtonPressCount==2){
        this->ui->menuConfig->setEnabled(true);
        this->configButtonTimer.stop();
        this->configButtonPressCount=0;
        return;
    }
    if(this->configButtonTimer.isActive()){
        this->configButtonPressCount++;
    }else{
        this->configButtonTimer.setSingleShot(true);
        this->configButtonTimer.start(2000);
    }
}

void MainWindow::exportData()
{

    //qDebug()<<"Data Captured.";
    this->ui->statusbar->showMessage(QString("%1: Process Finished.").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    if(this->ui->start_pushButton->text()=="Stop"){
        this->ui->start_pushButton->click();
        this->ui->email_pushButton->setEnabled(true);
    }
    if(!this->fileDir.isEmpty()){
        QDir dir(this->fileDir);
        if (!dir.exists()) {
            if(!dir.mkpath(".")){
                this->fileDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
                QDir dir(this->fileDir);
                if (!dir.exists()) {
                    dir.mkpath(".");
                }
            }
        }
    }else{
        this->fileDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QDir dir(this->fileDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    QFile file;
    /*
    if(this->fileDir.isEmpty()){
        fileDir = QFileDialog::getExistingDirectory(this,"Select The Directory Where You Want to Save the CSV Files.");
        if(fileDir.isEmpty()){
            this->ui->statusbar->showMessage("Directory Didn't Selected. File wasn't saved.");
            return;
        }
    }
    */
    this->fileName = fileDir+"/FECNCT_"+this->generalInfoMap["Serial Number"].mid(7,5)+QDateTime::currentDateTime().toString("_yyyy_MM_dd_hh_mm") +".csv";
    file.setFileName(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        if(this->logactive){
            this->ui->log_textEdit_2->append("\n["+QTime::currentTime().toString()+"]: "+tr("Error Opening File %1.").arg(fileName));
        }
        this->ui->statusbar->showMessage(QString("Error Opening File %1.").arg(fileName));
        return;
    }
    QTextStream out(&file);
    //Export data from All Tables using a loop
    QTableWidgetItem *curItem;
    for(int j=0;j<this->tables.length();j++){
        //Export data from tables
        QTableWidget *curTable = this->tables.at(j);
        out << QString("[%1]").arg(curTable->objectName())<<endl;
        for(int k=0;k<curTable->columnCount();k++){
            curItem = curTable->horizontalHeaderItem(k);
            out << QString("%1,").arg(curItem->text());
        }
        out << tr("Row Color");
        out<<endl;
        for(int m=0;m<curTable->rowCount();m++){
            //Export items in visual column order
            for(int n=0;n<curTable->columnCount();n++){
                curItem = curTable->item(m,n);
                //qDebug()<<QString("Header: %1").arg(curTable->horizontalHeaderItem(n)->text());
                out << QString("%1,").arg(curItem->text());
            }
            //Export Row Color
            out << "#FFFFFF"; //White Color
            out<<endl;
        }
        out<<endl;
        if(this->logactive){
            this->ui->log_textEdit_2->append("\n["+QTime::currentTime().toString()+"]: "+tr("%1 Records Successfully Exported From Table %2 to File %3").arg(curTable->rowCount()).arg(curTable->objectName()).arg(fileName));
        }
    }
    //Close File
    file.close();
}
void MainWindow::saveUserPreferences(){
    QSettings toSave(QSettings::IniFormat,QSettings::UserScope,VER_COMPANYNAME_STR,VER_FILEDESCRIPTION_STR);
    toSave.beginGroup("MainWindow");
    if(!this->isMaximized()){
        toSave.setValue("size",this->size());
        toSave.sync();
        toSave.setValue("pos",this->pos());
        toSave.sync();
    }
    toSave.setValue("maximized",this->isMaximized());
    toSave.sync();
    toSave.endGroup();
    toSave.beginGroup("LogBox");
    toSave.setValue("logboxAction",this->ui->actionShow_Log_Box->isChecked());
    toSave.sync();
    toSave.setValue("active",this->logactive);
    toSave.sync();
    toSave.endGroup();
    toSave.beginGroup("File");
    toSave.setValue("fileDir",this->fileDir);
    toSave.sync();
    toSave.endGroup();
    toSave.beginGroup("EmailConfig");
    toSave.setValue("serverName",this->serverName);
    toSave.sync();
    toSave.setValue("serverPort",this->port);
    toSave.sync();
    toSave.setValue("userName",this->username);
    toSave.sync();
    toSave.setValue("password",this->password);
    toSave.sync();
    toSave.setValue("recipient",this->rcpt);
    toSave.sync();
    toSave.setValue("subject",this->subject);
    toSave.sync();
    toSave.setValue("message",this->message);
    toSave.sync();
    toSave.endGroup();
}
void MainWindow::loadUerPreferences(){
    QSettings toLoad(QSettings::IniFormat,QSettings::UserScope,VER_COMPANYNAME_STR,VER_FILEDESCRIPTION_STR);
    toLoad.beginGroup("MainWindow");
    this->resize(toLoad.value("size", QSize(500, 500)).toSize());
    this->move(toLoad.value("pos", QPoint(200, 200)).toPoint());
    if(toLoad.value("maximized", false).toBool())
        this->showMaximized();
    toLoad.endGroup();
    toLoad.beginGroup("LogBox");
    //Show Log Box active the log for default
    this->ui->actionShow_Log_Box->setChecked(toLoad.value("logboxAction",false).toBool());
    //Click stop buttom in case of log disabled
    if(!toLoad.value("active",true).toBool()){
        this->ui->log_pushButton_2->click();
    }
    toLoad.endGroup();
    //Information to Timeout
    toLoad.beginGroup("NetworkConfig");
    this->requestTimeOut.setInterval(toLoad.value("timeoutValue", 14*1000).toInt()*1000);//Set Timeout Based on Network Config Dialog
    this->customAddress = toLoad.value("customAddress", "http://192.168.240.1:80").toString()+"/gainspan/profile/mcu";
    toLoad.endGroup();
    toLoad.beginGroup("File");
    this->fileDir = toLoad.value("fileDir",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    toLoad.endGroup();
    toLoad.beginGroup("EmailConfig");
    this->serverName = toLoad.value("servername","smtp.gmail.com").toString();
    this->port = toLoad.value("serverPort",465).toInt();
    this->username = toLoad.value("username","subdrivedatalogger@gmail.com").toString();
    this->password = toLoad.value("password","subdr1v3").toString();
    this->rcpt = toLoad.value("repipient","DLynn@fele.com").toString();
    this->subject = toLoad.value("subject","SubDrive Simple Data Logger").toString();
    this->message = toLoad.value("message","").toString();
    toLoad.endGroup();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveUserPreferences();
    event->accept();

}
void MainWindow::on_email_pushButton_clicked()
{
    QStringList files;
    QFile tmp_file;
    tmp_file.setFileName(this->fileName);
    if(tmp_file.open(QIODevice::ReadOnly)){
        files<<this->fileName;
        tmp_file.close();
    }
    //qDebug()<<files;
    Smtp* smtp = new Smtp(this->username, this->password, this->serverName, this->port);
    connect(smtp, SIGNAL(status(QString)), this, SLOT(mailSent(QString)));
    QObject::connect(smtp,SIGNAL(log(QString)),this,SLOT(updateLog(QString)));
    this->ui->statusbar->showMessage("Sending Log...");
    if(!files.isEmpty())
        smtp->sendMail(this->username, this->rcpt , this->subject,this->message, files );
    else
        smtp->sendMail(this->username, this->rcpt , this->subject,this->message);
    this->fileName="";
    this->ui->statusbar->clearMessage();
}

void MainWindow::mailSent(QString status)
{
    if(status == "Message sent")
        QMessageBox::warning( 0, this->windowTitle(), tr( "Message sent!\n\n" ) );
}
void MainWindow::showEmailConfigDialog(){
    emailConfig config;
    config.setServerName(this->serverName);
    config.setPort(this->port);
    config.setUserName(this->username);
    config.setPassword(this->password);
    config.setRcpt(this->rcpt);
    config.setSubject(this->subject);
    config.setMessage(this->message);
    config.setFileDirectory(this->fileDir);
    if(config.exec()){
        this->serverName = config.getServerName();
        this->port = config.getPort();
        this->username = config.getUserName();
        this->password = config.getPassword();
        this->rcpt = config.getRcpt();
        this->subject = config.getSubject();
        this->message = config.getMessage();
        this->fileDir = config.getFileDirectory();
    }
}
void MainWindow::updateLog(QString status){
    if(this->logactive){
        this->ui->log_textEdit_2->append("\n["+QTime::currentTime().toString()+"]:"+status);
    }
}

void MainWindow::on_exit_pushButton_clicked()
{
    this->close();
}
