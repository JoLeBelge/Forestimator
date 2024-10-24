#ifndef MATAPTCS_H
#define MATAPTCS_H

#include "matapt.h"
#include "cdicoapt.h"
#include "Wt/WCssDecorationStyle.h"
#include "Wt/WTemplate.h"
#include "Wt/WProgressBar.h"

extern bool globTest2;
using namespace  Wt;

class matAptCS : public Wt::WContainerWidget
{
public:
    matAptCS(cDicoApt * aDicoApt);

private:
     cDicoApt * mDicoApt;
    // un tableau 1 colonne qui liste les essences groupées par classes des 7 valeurs d'aptitude
     Wt::WTable * mAptTable;
     WComboBox * zbioSelection_;
     WContainerWidget * contListeUS, *contFicheUS;
     void hoverBubble(WContainerWidget *c, bool hover);
     void changeZbio();
     void updateListeUS();
     void showFicheUS(int US,std::string aVar);
     void displayNiche(std::string aEssCode);
     void resetNiche();

     std::vector<std::vector<cEss*>> mVEss;
     int zbio_;
     int US_;
     std::string mVar_;
     zbioPainted * graphZbio;
     std::map<std::tuple<int,std::string>, Wt::WPushButton *> mMapButtonUS;
};
#endif // MATAPTCS_H
