#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H

#include <iostream>
#include "grouplayers.h"
#include "parcellaire.h"
#include "uploadcarte.h"
#include "stackinfoptr.h"
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

#include <Wt/WIntValidator.h>
#include <Wt/WLineEdit.h>
using namespace Wt;

class cWebAptitude : public WContainerWidget
{
public:
  /*!\brief Instantiate a new form example.
   */
  cWebAptitude(Wt::WApplication* app);

  void handlePathChange();
  void login();//pour accès au mode expert

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

  WMenuItem * menuitem_presentation,* menuitem_carto, * menuitem_analyse;


  Wt::WStackedWidget * top_stack;// celui qui navige entre la page de garde (home), la page de présentation et les volets analyse/carto
  Wt::WStackedWidget * sub_stack;// celui qui navige entre la carte et la page de visu des résultats d'analyse sur un parcellaire
private:
  //std::map<std::string,cEss>  mMEss;
  Wt::WApplication* m_app;
  cDicoApt * mDico;

  stackInfoPtr * mStackInfoPtr;

  /*Wt::WTable                 *mClassifTable;
  std::string currentClassifST; // 2 modes de classification des stations forestières ; FEE et CS
  std::vector<WText *> clasLabels_;
  void changeClassClick(WText *t);
  void changeRasterClick(WText *t);
  void gotXY(double x, double y);
  */
  //Wt::Signal<std::string>& changeClass() { return changeClass_; }
  //Wt::Signal<std::string> changeClass_;

  // la table qui liste toutes les essences. pour le moment c'est une table.
  //Wt::WTable                 *mEssTable;
  // une manière de savoir qu'elle est l'essence active; s'inspirer de l'onglet "langague"
  //std::string currentEssCode;
  //int currentEssIndex;
  //void changeEssClick(WText *t);
 // Wt::Signal<std::string>& changeEss() { return changeEss_; }
 // Wt::Signal<std::string> changeEss_;
  //std::vector<WText *> essLabels_; // ce sont les labels qui sont dans le tableau
    groupLayers * mGroupL;
    parcellaire * mPA;
    uploadCarte * mUpload;


};

#endif // CWEBAPTITUDE_H
