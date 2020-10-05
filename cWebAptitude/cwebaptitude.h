#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H
#pragma once
#include <iostream>
#include "grouplayers.h"
#include "parcellaire.h"
#include "uploadcarte.h"
#include "stackinfoptr.h"
#include "auth.h"
//#include "main.h"
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
//#include <Wt/WLabel.h>
#include "wopenlayers.h"
#include "cdicoapt.h"
//#include <boost/range/adaptor/map.hpp>
//#include <boost/range/adaptors.hpp>
//#include <boost/foreach.hpp>
#include "statwindow.h"
#include "simplepoint.h"

#include <Wt/WIntValidator.h>
#include <Wt/WLineEdit.h>

#include "presentationpage.h"

//using namespace Wt;

class AuthApplication;

class cWebAptitude : public Wt::WContainerWidget
{
public:

    cWebAptitude(AuthApplication *app, Wt::Auth::AuthWidget* authWidget_);

    void handlePathChange();

    /*~cWebAptitude(){
    std::cout << "destructeur de cWebAptitude " << std::endl;

    //m_app=NULL;

    printf("Delete mGroupL cweb\n");
    //delete mGroupL;
    printf("Delete mPa cweb\n");
    //if(mPA) delete mPA;
    printf("Delete mUpload cweb\n");
    if(mUpload) mUpload=NULL;

    printf("Delete ... cweb\n");
    stack_info=NULL;
    menuitem2_analyse=NULL;

    printf("Delete mMap cweb\n");
    //if(mMap) delete mMap;
    mMap=NULL;
    printf("Delete mDico cweb\n");
    //if(mDico) delete mDico;
    mDico=NULL;
    printf("Delete m_app cweb\n");
    m_app=NULL;
    printf("cWeb done\n");
  }*/

    WOpenLayers * mMap;

    //WMenuItem * menuitem_presentation,* menuitem_carto, * menuitem_analyse;
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
    simplepoint * mSP;
    uploadCarte * mUpload;

};

#endif // CWEBAPTITUDE_H
