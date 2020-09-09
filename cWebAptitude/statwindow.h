#ifndef STATWINDOW_H
#define STATWINDOW_H
#include "grouplayers.h"
#include "layerstatchart.h"


class statWindow : public Wt::WContainerWidget
{
public:
    // a besoin du dictionnaire pour créer le layer qui contient la carte IGN pour faire la carte de situation globale
    statWindow(cDicoApt * aDico);

    void vider();
    void titre(std::string aTitre){mTitre->setText(aTitre);}
    void add1Aptitude(layerStatChart * lstat);
    void add1layerStat(Wt::WContainerWidget * layerStat);
    void generateGenCarte(OGRFeature *poFeature);

private:
    cDicoApt * mDico;

    Wt::WVBoxLayout * layout;
    //cDicoApt * mDico;
    WTable * mAptTable;
    WText * mTitre;

    WContainerWidget * mCarteGenCont;

    std::vector<WContainerWidget *> mVContStatIndiv;
    Layer * mIGN;

};

#endif // STATWINDOW_H
