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

     WContainerWidget * contUS;

     void hoverBubble(WContainerWidget *c, bool hover);
     void changeZbio();
     void updateUS();
     void updateApt(int US,std::string aVar);
     //void initAptTable(std::string aNTNHTitle);

     std::vector<std::vector<std::shared_ptr<cEss>>> mVEss;
     int zbio_;
     int US_;
     std::string mVar_;
     zbioPainted * graphZbio;

};
#endif // MATAPTCS_H
