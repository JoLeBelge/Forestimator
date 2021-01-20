#ifndef AUTH_H
#define AUTH_H


#pragma once
#include <stdio.h>
#include "Session.h"


#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>
#include <sqlite3.h>
#include <algorithm>

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/Auth/AuthWidget.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WMenuItem.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPopupMenuItem.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WEnvironment.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WToolBar.h>
#include <Wt/WTemplate.h>
#include <Wt/WString.h>
#include <sys/stat.h>

#include "api/stationdescresource.h"


class cWebAptitude; // forward declaration

class AuthApplication : public Wt::WApplication
{
public:
    AuthApplication(const Wt::WEnvironment& env, cDicoApt * dico);
    void authEvent();
    bool isLoggedIn();
    void logout();
    Wt::Auth::User getUser();
    cDicoApt * getDico(){return mDico;}

    Wt::Auth::AuthWidget* authWidget_;
    cWebAptitude * cwebapt;
private:
    cDicoApt * mDico;
    Session session_;
    bool loaded_=false; // sert à éviter que void authEvent ne crash si refresh la page et que user connecté...
};


#endif // AUTH_H
