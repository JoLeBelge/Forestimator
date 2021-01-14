#ifndef MAIN_H
#define MAIN_H


//#pragma once
/*#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>
#include <stdio.h>
#include <sqlite3.h>
#include <algorithm>*/

#include "auth.h"
#include "Session.h"
//#include "cwebaptitude.h"

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
/*#include <Wt/WContainerWidget.h>
#include <Wt/WMenuItem.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPopupMenuItem.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WBootstrapTheme.h>

#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WToolBar.h>
#include <Wt/WTemplate.h>
#include <Wt/WString.h>
#include <sys/stat.h>*/

#include <Wt/WServer.h>
#include <Wt/Auth/AuthWidget.h>
#include <Wt/Auth/PasswordService.h>

#include "api/stationdescresource.h"

//class AuthApplication;

std::unique_ptr<Wt::WApplication> createAuthApplication(const Wt::WEnvironment &env);
std::unique_ptr<Wt::WApplication> createForestimatorAPI(const Wt::WEnvironment &env);
int main(int argc, char **argv);

#endif // MAIN_H
