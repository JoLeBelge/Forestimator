#ifndef STATWINDOW_H
#define STATWINDOW_H
#include "grouplayers.h"
#include "layerstatchart.h"
#include "auth/auth.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <Wt/Render/WPdfRenderer.h>
#include <Wt/WApplication.h>
//#include "iostream"
//#include "wkhtml/wkhtmlutil.h"

class statWindow;




class statWindow : public Wt::WContainerWidget
{
public:
    // a besoin du dictionnaire pour créer le layer qui contient la carte IGN pour faire la carte de situation globale
    statWindow(groupLayers * aGL);

    void vider();
    void titre(std::string aTitre){mTitre->setText(aTitre);}
    void add1Aptitude(layerStatChart * lstat);
    void add1layerStat(layerStatChart *layerStat);
    void add1layerStat(statHdom *lStatCont);
    void genIndivCarteAndAptT();
    void generateGenCarte(OGRFeature *poFeature);

    //void export2pdf(std::string img, int length);
    void export2pdf();
    void renderPdf(Wt::Render::WPdfRenderer * renderer);

    WText * mTitre;
    WTable * mAptTable;
    WPushButton * createPdfBut;
    WContainerWidget * mCarteGenCont;
    WContainerWidget * mAllStatIndivCont;

private:
    cDicoApt * mDico;
    AuthApplication * mApp;
    groupLayers*mGL;

    Wt::WVBoxLayout * layout;

    // pour la carte de localisation
    std::shared_ptr<layerBase> mIGN;
    // pour les information générales
    std::shared_ptr<layerBase> mMNT, mZBIO, mPente;

    //olOneLay *olStatic;

    /*  Signal pour récupérer l'image en base float pour générer un PDF
    JSlot slotImgPDF;
    JSignal<std::string, int>  sigImgPDF;
    int chunkImgPDF=0;
    int chunkImgPDFind=0;
    std::string strImgPDF;*/
};

#endif // STATWINDOW_H
