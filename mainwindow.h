/**
 * @file mainwindow.h
 * This Class defines application Main Window.
 * @brief Header file for MainWindow class definition.
 *
 * This class contains code related to application Main Window and most of the code related to application operation.
 *
 * @author Plinio Andrade &lt;PAndrade@fele.com&gt;
 * @version 1.0.0.0 (Qt: 5.3.1)
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
//#include <QScrollerProperties>
#include <QBrush>
#include <QNetworkConfiguration>
#include <QHash>
class QDomDocument;
class QNetworkAccessManager;
class QNetworkReply;
class QTableWidget;
class QScroller;
class NetworkConfigDialog;
namespace Ui {
class MainWindow;

}

/**
 * @brief The MainWindow class defines application Main Window.
 *
 * \image html mainwindow-dialog.png
 * This class contains code related to application Main Window and most of the code related to application operation.
 *
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs this class with the given parent widget.
     *
     * @param parent A pointer to parent widget.
     */
    explicit MainWindow(QWidget *parent = 0);
    /**
     * @brief Destructs this class.
     *
     */
    ~MainWindow();
private slots:
    void updateLog(QString status);

    /**
     * @brief Shows application About dialog.
     *
     * Shows application About dialog showing the dialog created by AboutDialog.
     *
     */
    void aboutApp();

    /**
     * @brief Requests data from network.
     *
     * This slot is called in intervals defined in requestTimer. However, the requestTimer is disconnected in this member
     * and connected again either timeoutTimer timeout() signal is emitted or this request is finished.
     *
     */
    void request();
    /**
     * @brief Aborts a request.
     *
     * Aborts a request and connects requestTimer to request() slot again. This slot is connected to timeoutTimer timeout() signal.
     */
    void abortRequest();
    /**
     * @brief Parses data received in request() member.
     *
     * Parses data from request() member, call the appropried member according to the group of data received, and connects
     * requestTimer to request() slot again.
     */
    void parseXML();  
    /**
     * @brief Changes visibility of Log Box.
     *
     * @param visible If true shows Dashborad tab, if false hides Dashboard tab.
     */
    void LogBoxVisible(bool visible);
    /**
     * @brief Shows application Network Configuration dialog.
     *
     * Shows application Network Configuration dialog showing the dialog created by NetworkConfigDialog.
     */
    void networkConfigDialogSlot();  
    /**
     * @brief This member is called always that Start button is clicked.
     *
     */
    void on_start_pushButton_clicked();
    /**
     * @brief This member is called always that Clear Log button is clicked.
     *
     */
    void on_clearlog_pushButton_clicked();
    /**
     * @brief This member is called always that Start Log/Stop Log button is clicked.
     *
     */
    void on_log_pushButton_2_clicked();
    void on_config_pushButton_clicked();
    void on_email_pushButton_clicked();

    //About Email
    void mailSent(QString status);
    void showEmailConfigDialog();   

    void on_exit_pushButton_clicked();

private:   
    //MainWindow Ui
    /**
     * @brief ui object was created by Qt Creator.
     *
     * It contains the graphical elements created using Qt Design.
     */
    Ui::MainWindow *ui;
    //About CSV File
    QString fileDir;
    QString curFile;
    QString fileName;
    QList<QTableWidget*> tables;
    //About Email
    QString serverName,username,password,subject,message,rcpt;
    int port;

    //About Scroller
    //QScrollerProperties scrollprop; /**< Properties for the scoll screen feature. For more information see QScroller documentation */

    //About Network
    QNetworkAccessManager * manager; /**< Pointer to network manager for this application. See QNetworkAccessManager documentation.*/
    QNetworkReply* reply; /**< Pointer to network reply for this application. See QNetworkReply documentation. */
    /**
     * @brief Timer for requisition interval.
     *
     *  The timeout() signal of this timer is connected to request() slot.
     */
    QTimer requestTimer;
    QTimer configButtonTimer;
    int configButtonPressCount;
    /**
     * @brief Request timout Timer
     *
     * The timeout() signal of this timer is connected to abortRequest() slot.
     */
    QTimer requestTimeOut;
    NetworkConfigDialog *networkConfigDialog; /**< NetworkConfigDialog class object. */
    QString customAddress; /**< This string contains the address that application will connect to. */
    //Helper variable for Request Timer
    bool stoptimer; /**< True if stop buttom was pressed and the requestTimer should be stopped. False otherwise. */
    //Tables Pointers
    QTableWidget *faulthistory_tableWidget,*generalinfo_tableWidget,*configevent_tableWidget,*reset_event_hist_tableWidget,*temp_event_hist_tableWidget,*motor_ontime_event_hist_tableWidget,*poweron_event_hist_tableWidget,*communication_event_hist_tableWidget,*current_limit_event_hist_tableWidget,*overload_event_hist_tableWidget;
    //Map of General Info data
    QHash<QString,QString> generalInfoMap;
    //Parsers Methods
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group0 XML file QDomDocument struct.
     */
    void parsegroup0(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group1 XML file QDomDocument struct.
     */
    void parsegroup1(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group2 XML file QDomDocument struct.
     */
    void parsegroup2(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group3 XML file QDomDocument struct.
     */
    void parsegroup3(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group4 XML file QDomDocument struct.
     */
    void parsegroup4(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group5 XML file QDomDocument struct.
     */
    void parsegroup5(QDomDocument* doc=0);
    /**
     * @brief Parses the given XML file sctruct in QDomDocument.
     *
     * @param doc Group6 XML file QDomDocument struct.
     */
    void parsegroup6(QDomDocument* doc=0);
    /**
     * @brief Decides what type of log is in the FaultEventInfo vector.
     *
     * This function will call a specific member function according to the log type.
     */
    void UpdateFaultEventText();
    /**
     * @brief Populates Fault History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToFaultTable();
    /**
     * @brief Populates Config Event History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToConfigChangeEventTable();
    /**
     * @brief Populates Reset Event History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToResetLogTable();
    /**
     * @brief Populates Temperature Evnet History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToTemperatureLimitTable();
    /**
     * @brief Populates Motor On Time Event History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToMotorOnEventTable();
    /**
     * @brief Populates Power On Time Event table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToPowerOnEventTable();
    /**
     * @brief Populates Communication Event History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToCommunicationsErrorLogTable();
    /**
     * @brief Populates Current Limit Event History table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToCurrentLimitTable();
    /**
     * @brief Populates Fixture Data Store Log table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToTestFixtureDataStoreLogTable();
    /**
     * @brief Populates Overload Event table with data from FaultEventInfo vector.
     *
     */
    void ConvertReceivedArrayDataToOverloadFixtureTable();
    /**
     * @brief Populates General Info table with data from General Info tab.
     *
     */
    void UpdateGeneralInfoTable();

    //Internal variables
    /**
     * @brief Creates all connections between signals and slots for this application.
     *
     */
    void makeconnections();
    int previous_group; /**< Contains the previous parsed XML group file */
    /**
     * @brief Contains data from network (XML parsed file).
     *
     * This vector is updated every new requisition and its items are used after the last requisition of the cycle to populate
     * the tables.
     */
    int FaultEventInfo[34];
    bool logactive; /**< True if the application log is active. False otherwise. */
    //About Tables 
    /**
     * @brief Creates the tables of this application.
     *
     */
    void createtables();   
    /**
     * @brief Converts date contained in the FaultEventInfo property vector to numbers.
     *
     * @return QList<int> The list with the date in Days, Hours, and Minutes.
     */
    QList<int> ConvertTimefromVector();
    /**
     * @brief Completes date columns in the given row in the table.
     *
     * Because most of tables have the first columns with date information this member is used to avoid rewriting the same code.
     *
     * @param curTable The given table.
     * @param thisCycle If true completes thisCycle date information.
     * @param logNumber The current log number. This number comes from network (XML parsed file).
     * @param curRow The given row.
     */
    void CompleteDateonTable(QTableWidget *curTable, bool thisCycle = true, int logNumber=0, int curRow=0, QStringList *being_added=0);
    /**
     * @brief Return the index of the given log number in the given table.
     *
     * If the table doesn't have a record with the given logNumber it will create a new row and return its index.
     *
     * @param curTable The given table.
     * @param logNumber The given log number.
     * @return int The record index.
     */
    int isthereSameLog(QTableWidget *curTable, int logNumber, QStringList *record=0);
    //Save/Load User Preferences members
    /**
     * @brief Saves user preferences.
     *
     * Saves user preferences such as window position, window size, user modified fields. This function is called always\n
     * that this windows is closed using the Ok button. For more information about how it works see the QSettings documentation.
     */
    void saveUserPreferences();
    /**
     * @brief Loads user preferences.
     *
     * Loads user preferences such as window position, window size, user modified fields. This function is called always\n
     * that this windows is closed using the Ok button. For more information about how it works see the QSettings documentation.
     */
    void loadUerPreferences();
    /**
     * @brief Window close event.
     *
     * This Event is called always that this windows is closed. This function is used to perform saving of user preferences.
     * @param event QCloseEvent object.
     */
    void closeEvent(QCloseEvent *event);

    //Log Number to set the limit of General Info Table items
    int generalInfoLogNumber; /**< Contains the current number of records in the General Info table */
    /**
     * @brief Contains the SSID of the current network configuration(QNetworkConfiguration).
     *
     * This property is used to add data to tables and to add data to correct graphics.
     *
     */
    QString curSSID;   
    int curNetworkListIndex; /**< Current index for selectedNetworks list. It's used for swapRoamingNetwork() member. */
    void exportData();
};
#endif // MAINWINDOW_H

