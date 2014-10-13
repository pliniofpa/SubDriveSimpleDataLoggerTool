/**
 * @file networkconfigdialog.h
 * This Class defines application Network Configuration dialog window.
 * @brief Header file for NetworkConfigDialog class definition.
 *
 * This dialog window is created through <B>Configuration->Network Config.</B> menu.
 *
 * @author Plinio Andrade &lt;PAndrade@fele.com&gt;
 * @version 1.0.0.0 (Qt: 5.3.1)
 */

#ifndef NETWORKCONFIGDIALOG_H
#define NETWORKCONFIGDIALOG_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
class NetworkConfigDialog;
}

/**
 * @brief The NetworkConfigDialog class defines application Network Configuration dialog window.
 *
 * \image html network-config-dialog.png
 * This dialog window is created through <B>Configuration->Network Config.</B> menu.
 */
class NetworkConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief ui object was created by Qt Creator.
     *
     * It contains the graphical elements created using Qt Design.
     */
    Ui::NetworkConfigDialog *ui;
    /**
     * @brief Constructs this class with the given parent widget.
     *
     * @param parent A pointer to parent widget.
     */
    explicit NetworkConfigDialog(QWidget *parent = 0);
    /**
     * @brief Destructs this class.
     *
     */
    ~NetworkConfigDialog();
    /**
     * @brief Returns the Address line edit text.
     *
     * @return QString The Address line edit text.
     */
    QString getAddress();
    /**
     * @brief Returns the Inverval spinbox value.
     *
     * @return int The Interval spinbox value.
     */
    int getTimeout();
    /**
     * @brief Returns the checked state (Checked/Unchecked) of Enabled checkbox.
     *
     * @return bool True if checkbox is checked. False if it's unchecked.
     */
    bool getcustomAddressCheckBoxState();
private slots:
    /**
     * @brief This member is called always that Address lineEdit text is changed.
     *
     * @param text The new text.
     */
    void addressTextChanged(QString text);
    /**
     * @brief Saves user preferences.
     *
     * Saves user preferences such as window position, window size, user modified fields. This function is called always
     * that this windows is closed using the Ok button. For more information about how it works see the QSettings documentation.
     */
    void saveUserPreferences();
    /**
     * @brief Custom Address checkbox class slot.
     *
     * This slot is connected to Custom Address checkbox and it's called always that Custum Address checkbox state (Checked/Unchecked)
     * is changed.
     *
     * @param checked True if checkbox is checked. False if it's unchecked.
     */
    void checkBoxToggled(bool checked);
private:
    /**
     * @brief Window show event.
     *
     * This Event is called always that this windows is showed. This function is used to perform loading of user preferences.
     * @param event QShowEvent object.
     */
    void showEvent(QShowEvent * event);    
    /**
     * @brief Loads user preferences.
     *
     * Loads user preferences such as window position, window size, user modified fields. This function is called always
     * that this windows is closed using the Ok button. For more information about how it works see the QSettings documentation.
     */
    void loadUerPreferences();
};

#endif // NETWORKCONFIGDIALOG_H
