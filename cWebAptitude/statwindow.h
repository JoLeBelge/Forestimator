#ifndef STATWINDOW_H
#define STATWINDOW_H
#include "grouplayers.h"
#include "layerstatchart.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <Wt/Render/WPdfRenderer.h>
#include <Wt/WApplication.h>
//#include "iostream"

#include "wkhtml/wkhtmlutil.h"

class ReportResource;
class statWindow;

std::string roundDouble(double d, int precisionVal=1);


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

    void export2pdf(std::string img);

    WText * mTitre;
    WTable * mAptTable;
    WPushButton * createPdfBut;
    WContainerWidget * mCarteGenCont;
private:
    cDicoApt * mDico;

    Wt::WVBoxLayout * layout;

    std::vector<WContainerWidget *> mVContStatIndiv;
    // pour la carte de localisation
    Layer * mIGN;
    // pour les information générales
    Layer * mMNT, * mZBIO, * mPente;

    /*  Signal pour récupérer l'image en base float pour générer un PDF     */
    JSlot slotImgPDF;
    JSignal<std::string>  sigImgPDF;

};

#endif // STATWINDOW_H
