/**
 * @file main.cpp
 * \brief The main file
 *
 * This file contais the main() fuction of this application. Basically the code here just creates a MainWindow object and shows it.
 * @author Plinio Andrade &lt;PAndrade@fele.com&gt;
 * @version 1.0.0.0 (Qt: 5.3.1)
 */
#include <QApplication>
#include "version.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(VER_FILEDESCRIPTION_STR);
    app.setOrganizationName(VER_COMPANYNAME_STR);
    app.setOrganizationDomain(VER_COMPANYDOMAIN_STR);
    app.setQuitOnLastWindowClosed(true);
    MainWindow mainWindow;
    mainWindow.show();
    exit(app.exec());
}
