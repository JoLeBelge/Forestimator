#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H
#pragma once
#include <iostream>
#include "grouplayers.h"
#include "parcellaire.h"
#include "uploadcarte.h"
#include "stackinfoptr.h"
#include "auth.h"
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WApplication.h>
#include <Wt/WText.h>
#include <Wt/WStringUtil.h>
#include <Wt/WWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLayout.h>
#include <Wt/WMenu.h>
#include <Wt/WTextArea.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WOverlayLoadingIndicator.h>
#include "wopenlayers.h"
#include "cdicoapt.h"
#include "statwindow.h"
#include "simplepoint.h"

#include <Wt/WIntValidator.h>
#include <Wt/WLineEdit.h>

#include "presentationpage.h"

class AuthApplication;

class cWebAptitude : public Wt::WContainerWidget
{
public:

    cWebAptitude(AuthApplication *app, Wt::Auth::AuthWidget* authWidget_);

    void handlePathChange();
    WOpenLayers * mMap;

    Wt::WNavigationBar * navigation;
    Wt::WStackedWidget * top_stack;// celui qui navige entre la page de garde (home), la page de présentation et les volets analyse/carto
    Wt::WStackedWidget * sub_stack;// celui qui navige entre la carte et la page de visu des résultats d'analyse sur un parcellaire
    Wt::Auth::AuthWidget* authWidget;
    Wt::WPushButton *b_login;
    groupLayers * mGroupL;
private:
    //std::map<std::string,cEss>  mMEss;
    //Wt::WApplication* m_app;
    AuthApplication * m_app;
    cDicoApt * mDico;
    stackInfoPtr * mStackInfoPtr;

    parcellaire * mPA;
    //simplepoint * mSP;// c'est groupGL qui s'en charge
    uploadCarte * mUpload;

};

#endif // CWEBAPTITUDE_H
