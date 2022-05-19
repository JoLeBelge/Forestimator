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
#include <Wt/WImage.h>
#include <Wt/WSvgImage.h>
#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPainter.h>

// gdal pour ouvrir un shp et le mettre dans un painted widget. expérimental.
#include "ogrsf_frmts.h"
#include "gdal_utils.h"

extern bool globTest2;
using namespace  Wt;

bool commonEss(std::string aCode, std::vector<std::shared_ptr<cEss>> & aV2);

// inspiré de staticMap, forestimator
class zbioPainted : public Wt::WPaintedWidget {
public:
    zbioPainted(std::string  aShp, std::shared_ptr<cdicoAptBase> aDico);
    ~zbioPainted(){
        if (mDS!=NULL){
            GDALClose(mDS);
        }
    }
    double xGeo2Im(double x);
    double yGeo2Im(double y);

    void drawPol(OGRPolygon * pol, Wt::WPainter * painter);
    void selectZbio(int zbio) {
    zbio_ = zbio;
    displayApt_=0;
    update();          // Trigger a repaint.
    }

    void displayAptMap(std::string essCode) {
    displayApt_=1;
    essCoce_=essCode;
    update();          // Trigger a repaint. Oui mais comment avoir deux type de repaint différent? un pour la sélection de la zone bioclim, l'autre pour la carte d'aptitude bioclimatique d'une essence? en utilisant une variable membre!
    }

protected:
    void paintEvent(Wt::WPaintDevice *paintDevice);

private:
    OGREnvelope * ext;
    // taille de l'image en pixel
    int mSx,mSy;
    // taille de l'emprise de l'image en mètre
    double mWx,mWy;
    int zbio_;
    bool displayApt_;
    std::string shpPath, essCoce_;
    std::shared_ptr<cdicoAptBase> mDico;
    OGRLayer * mlay;
    GDALDataset * mDS;
};

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

     void hoverBubble(WContainerWidget *c, bool hover);
     void clicEco(std::tuple<int,int> ntnh);
     void trierEss(std::tuple<int,int> ntnh, int zbio);
     void trierEss(std::tuple<int,int> ntnh, int zbio, std::vector<std::vector<std::shared_ptr<cEss>>> * aVEss);
     void changeZbio();
     void displayNiche(std::string aEssCode);
     void resetEco();
     void displayMatApt();
     void initAptTable(std::string aNTNHTitle);
     void selectLevel4comparison(std::tuple<int,int> ntnh){
         std::vector<std::tuple<int,int>> Vntnh;
         std::tuple<int,int> ntnhBase(nt_,nh_);
         Vntnh.push_back(ntnhBase);
         Vntnh.push_back(ntnh);
         compareMatApt(Vntnh);
     }
     void compareMatApt(std::vector<std::tuple<int,int>> aVntnh);

     void filterMouseEvent(WMouseEvent event, std::tuple<int,int> ntnh){
         if (event.modifiers().test(Wt::KeyboardModifier::Shift)){
             selectLevel4comparison(ntnh);
         } else {
             clicEco(ntnh);
         }
     }

     void getVEssCommun(std::vector<std::shared_ptr<cEss>> aV1,std::vector<std::shared_ptr<cEss>> aV2, std::vector<std::shared_ptr<cEss>> & aVCom, std::vector<std::shared_ptr<cEss>> & aVDiff);

     std::vector<std::vector<std::shared_ptr<cEss>>> mVEss;
     std::map<std::tuple<int,int>, Wt::WContainerWidget *> mMapCircleEco;
     int zbio_, nt_,nh_;
     zbioPainted * graphZbio;
};


#endif // MATAPT_H
