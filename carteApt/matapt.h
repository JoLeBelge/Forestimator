#ifndef MATAPT_H
#define MATAPT_H
#include "cdicoaptbase.h"
#include "Wt/WContainerWidget.h"
#include "Wt/WTable.h"
#include "Wt/WLayout.h"
#include "Wt/WHBoxLayout.h"
#include "Wt/WVBoxLayout.h"
#include "Wt/WText.h"
#include "Wt/WAnchor.h"
#include "Wt/WComboBox.h"
#include "Wt/WMessageBox.h"
#include "Wt/WBreak.h"

extern bool globTest2;
using namespace  Wt;

class matApt : public Wt::WContainerWidget
{
public:
    matApt(std::shared_ptr<cdicoAptBase> aDicoApt);
    // recevoir les valeurs de prédiction NT ou NH issue de la RF
    void receivePrediction(int aCode,std::vector<double> aVPredNT,std::vector<double> aVPredNH);

private:
    std::shared_ptr<cdicoAptBase> mDicoApt;

    // un tableau pour l'écogramme
    Wt::WTable * mEco;
    // un tableau croisé pour l'aptitude bioclim et hydro-trophique pour un niveau donné
     Wt::WTable * mAptTable;
     WComboBox * zbioSelection_;

     void hoverEco(WContainerWidget *c, bool hover);
     void clicEco(std::tuple<int,int> ntnh);
     void trierEss(std::tuple<int,int> ntnh, int zbio);
     void changeZbio();

     std::vector<std::vector<std::shared_ptr<cEss>>> mVEss;
     std::map<std::tuple<int,int>, Wt::WContainerWidget *> mMapCircleEco;
     int zbio_;
};

#endif // MATAPT_H
