#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H
#pragma once
#include <iostream>
#include "grouplayers.h"
#include "parcellaire.h"
#include "uploadcarte.h"
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
class parcellaire;
class uploadCarte;

class cWebAptitude : public Wt::WContainerWidget
{
public:

    cWebAptitude(AuthApplication *app, Wt::Auth::AuthWidget* authWidget_);

    void handlePathChange();
    WOpenLayers * mMap;

    Wt::WNavigationBar * navigation;
    Wt::WStackedWidget * top_stack;// celui qui navige entre la page de garde (home), la page de présentation et les volets analyse/carto
    Wt::Auth::AuthWidget* authWidget;
    Wt::WPushButton *b_login;
    groupLayers * mGroupL;

    //WStackedWidget * stack_info; // cause que je dois changer de current index après avoir mis à jour la légende que je clique sur une station
    WContainerWidget * mSimplepointW;
    WContainerWidget * mGroupLayerW;
    WMenuItem * menuitem_analyse,* menuitem_app,*menuitem_legend,*menuitem_documentation,*menuitem_simplepoint,*menuitem_login,*menuitem_panier;
    WDialog *dialog_anal,*dialog_info,*dialog_catalog;
private:
    void load_content_couches(WContainerWidget * content);
    //std::map<std::string,cEss>  mMEss;
    //Wt::WApplication* m_app;
    AuthApplication * m_app;
    cDicoApt * mDico;

    parcellaire * mPA;
    //simplepoint * mSP;// c'est groupGL qui s'en charge
    uploadCarte * mUpload;

};

#endif // CWEBAPTITUDE_H
