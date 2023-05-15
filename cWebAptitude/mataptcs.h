#ifndef MATAPTCS_H
#define MATAPTCS_H

#include "matapt.h"
#include "cdicoapt.h"

extern bool globTest2;
using namespace  Wt;

class matAptCS : public Wt::WContainerWidget
{
public:
    matAptCS(cDicoApt * aDicoApt);

private:
    cDicoApt * mDicoApt;

    // un tableau 3 colonne pour les aptitudes
     Wt::WTable * mAptTable;
     WComboBox * zbioSelection_;

     void hoverBubble(WContainerWidget *c, bool hover);
     void changeZbio();
     void updateApt(int US);
     //void initAptTable(std::string aNTNHTitle);

     std::vector<std::vector<std::shared_ptr<cEss>>> mVEss;
     int zbio_, US_;
     zbioPainted * graphZbio;

};
#endif // MATAPTCS_H
