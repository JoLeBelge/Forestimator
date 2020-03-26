#ifndef CWEBAPTITUDE_H
#define CWEBAPTITUDE_H

#include "grouplayers.h"
#include <Wt/WContainerWidget.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WApplication.h>
#include <Wt/WText.h>
#include <Wt/WStringUtil.h>
#include <Wt/WWidget.h>
#include <Wt/WTable.h>
#include <Wt/WLayout.h>
//#include <Wt/WLabel.h>
#include "wopenlayers.h"
#include "cdicoapt.h"
//#include <boost/range/adaptor/map.hpp>
//#include <boost/range/adaptors.hpp>
//#include <boost/foreach.hpp>
using namespace Wt;

class cWebAptitude : public WContainerWidget
{
public:
  /*!\brief Instantiate a new form example.
   */
  cWebAptitude(Wt::WApplication* app);
  WOpenLayers * mMap;
private:
  //std::map<std::string,cEss>  mMEss;
  Wt::WApplication* m_app;
  cDicoApt * mDico;

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


};

#endif // CWEBAPTITUDE_H
