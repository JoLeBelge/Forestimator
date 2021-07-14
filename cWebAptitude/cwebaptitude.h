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
#include "panier.h"
#include "analytics.h"

class AuthApplication;
class parcellaire;
class uploadCarte;
class panier;

class dialog : public Wt::WDialog
{
public:
    dialog(const WString& windowTitle, Wt::WMenuItem * aMenu);

    void myshow(){
        if (mShow){show();}
    }

private:
    // c'est le pointeur vers le boutton du menu qui va permettre d'afficher ou cacher la fenetre
    Wt::WMenuItem * mMenu;
    // permet de savoir si on réaffiche une fenetre quand on reviens sur la page carto après l'avoir quitté pour documentation par ex.
    bool mShow;
    Wt::WColor col_sel = Wt::WColor(23,87,23);
    Wt::WColor col_not_sel = Wt::WColor("transparent");
};

class cWebAptitude : public Wt::WContainerWidget
{
public:

    cWebAptitude(AuthApplication *app, Wt::Auth::AuthWidget* authWidget_);

    void handlePathChange();

    /** VARS GLOBALES  **/
    WOpenLayers * mMap;

    Wt::WNavigationBar * navigation;
    Wt::WStackedWidget * top_stack;// celui qui navige entre la page de garde (home), la page de présentation et les volets analyse/carto
    Wt::Auth::AuthWidget* authWidget;
    Wt::WPushButton *b_login;
    groupLayers * mGroupL;

    //WStackedWidget * stack_info; // cause que je dois changer de current index après avoir mis à jour la légende que je clique sur une station
    WContainerWidget *mSimplepointW, *mGroupLayerW, *mLegendW;
    WMenuItem * menuitem_analyse,* menuitem_app,*menuitem_legend,*menuitem_documentation,*menuitem_simplepoint,*menuitem_login,*menuitem_panier,*menuitem_catalog,*menuitem_cadastre;
    dialog *dialog_anal,*dialog_info,*dialog_catalog,*dialog_cadastre,*dialog_legend;
    panier * mPanier;

    // cacher tout les dialogues ou les rendre visibles lorsqu'on change de page, ex vers documentation
    void showDialogues(bool b=true){
        if (b){
          dialog_anal->myshow();
          dialog_info->myshow();
          dialog_cadastre->myshow();
          dialog_legend->myshow();
          dialog_catalog->myshow();
        } else {
            dialog_anal->hide();
            dialog_info->hide();
            dialog_cadastre->hide();
            dialog_legend->hide();
            dialog_catalog->hide();
        }
    }
private:
    void load_content_couches(WContainerWidget * content);
    //std::map<std::string,cEss>  mMEss;
    //Wt::WApplication* m_app;
    AuthApplication * mApp;
    cDicoApt * mDico;

    parcellaire * mPA;
    //simplepoint * mSP;// c'est groupGL qui s'en charge
    uploadCarte * mUpload;

    Wt::WColor col_sel = Wt::WColor(23,87,23);
    Wt::WColor col_not_sel = Wt::WColor("transparent");
};

#endif // CWEBAPTITUDE_H
