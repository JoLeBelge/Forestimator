#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H
#pragma once

#include <Wt/WBootstrap5Theme.h>
#include <stdio.h>
#include "Session.h"
#include <iostream>
#include "grouplayers.h"
#include "parcellaire.h"
//#include "uploadcarte.h"
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WApplication.h>
#include <Wt/Auth/AuthWidget.h>
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
#include "Wt/WEnvironment.h"

#include <Wt/WIntValidator.h>
#include <Wt/WLineEdit.h>

#include "presentationpage.h"
#include "panier.h"
#include "analytics.h"

#include <curl/curl.h>


using namespace std;
using namespace Wt;

#include "../stationDescriptor/rapidxml/rapidxml.hpp"
using namespace rapidxml;

extern bool globTest;

class parcellaire;
class panier;


class dialog : public Wt::WDialog
{
public:
    dialog(const WString& windowTitle, Wt::WMenuItem * aMenu,const WEnvironment * env);

    void myshow(){
        // on ne peut pas utiliser l'environnement dans le constructeur, car au début de la session, la taille de l'écran n'est pas encore définie (plain html session without ajax machin)
        int w_=env_->screenWidth()*5.0/10.0;
        int h_=env_->screenHeight()*7.0/10.0;
        //std::cout << " set size dialog " << w_ << " , " << h_ << std::endl;
        // setMaximumSize(w_,h_);

        setMaximumSize(w_,h_);
        if (mShow){show();}
    }

private:
    // c'est le pointeur vers le boutton du menu qui va permettre d'afficher ou cacher la fenetre
    Wt::WMenuItem * mMenu;
    // permet de savoir si on réaffiche une fenetre quand on reviens sur la page carto après l'avoir quitté pour documentation par ex.
    bool mShow;
    Wt::WColor col_sel = Wt::WColor(23,87,23);
    Wt::WColor col_not_sel = Wt::WColor("transparent");
    const WEnvironment * env_;
};

class cWebAptitude : public Wt::WApplication
{
public:

    ~cWebAptitude(){delete mGroupL;}
    cWebAptitude(const Wt::WEnvironment& env, cDicoApt * dico);
    void loadStyles();
    std::unique_ptr<Wt::Auth::AuthWidget> loadAuthWidget();
    void authEvent();
    bool isLoggedIn();
    void logout();
    Wt::Auth::User getUser();

    void addLog(std::string page,typeLog cat=typeLog::page);   // ajoute un record aux stat web

    cDicoApt * mDico;
    Analytics mAnal;
    Wt::WDialog *dialog_auth;

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

    // cacher tout les dialogues ou les rendre visibles lorsqu'on change de page, ex vers documentation ou vers fenetre authentification
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

    // un bon référencement dans moteur de recherche google passe par un header avec une description et un titre propre à chaque page. géré ici
    void changeHeader(std::string aSection);

      parcellaire * mPA;
private:
    void load_content_couches(WContainerWidget * content);

    Session session_;
    bool loaded_=false; // sert à éviter que void authEvent ne crash si refresh la page et que user connecté...

    Wt::WColor col_sel = Wt::WColor(23,87,23);
    Wt::WColor col_not_sel = Wt::WColor("transparent");
};

#endif // CWEBAPTITUDE_H
