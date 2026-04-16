#ifndef LEGEND_H
#define LEGEND_H
//#include "grouplayers.h"
//#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WMenuItem.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WLabel.h>
#include <Wt/WString.h>
#include "ecogrammeEss.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Render/WPdfRenderer.h>
#include "iostream"
#include <Wt/WRasterImage.h>
#include "cwebaptitude.h";

class EcogrammeEss;
class simplepoint;
class cWebAptitude;
class ST;
class Layer;
using namespace Wt;
class color;
class MyRenderer; // dérivé de classe WPdfRenderer pour laquelle on ajoute des footers, voir https://redmine.webtoolkit.eu/boards/2/topics/14334

enum TypeClassifST
{
    FEE,
    CS
};


namespace {
    void HPDF_STDCALL error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no,
               void *user_data) {
    fprintf(stderr, "libharu error: error_no=%04X, detail_no=%d\n",
        (unsigned int) error_no, (int) detail_no);
    }
}

class simplepoint: public WContainerWidget
{
public:
    simplepoint(cWebAptitude * app);
    void vider();
    void initStation();
    void add1InfoRaster(std::vector<std::string> aV);
    void detailCalculAptFEE();
    void afficheAptAllEss();
    void extractInfo(double x, double y);
    // retourne les aptitudes des essences pour une position donnée (click sur la carte)
    // key ; code essence. Value ; code aptitude
    std::map<std::string, int> apts(TypeClassifST aType);

    std::vector<std::string> displayInfo(std::shared_ptr<layerBase> al);

    Wt::WText                 *mIntroTxt;
    Wt::WTable                 *mInfoT;
    Wt::WTable                 *mDetAptFEE;
    Wt::WTable                 *mAptAllEss;
    WPushButton * createPdfBut;
    EcogrammeEss       *mEcoEss;
    Wt::WContainerWidget     * mContEco;
    cWebAptitude *m_app;
    //Wt::WText                  *titre_;
    cDicoApt * mDico;

    std::string NH(){ return mDico->NH(mNH);}
    std::string NT(){ return mDico->NT(mNT);}
    std::string TOPO(){ return mDico->TOPO(mTOPO);}
    std::string ZBIO(){ return mDico->ZBIO(mZBIO);}
    std::string STATION(){return mDico->station(mZBIO,mSt);}

    bool hasNH(){ return (mDico->NH()->find(mNH)!=mDico->NH()->end()) && mNH!=0;}
    bool hasNT(){ return mDico->NT()->find(mNT)!=mDico->NT()->end();}
    bool hasZBIO(){ return mDico->ZBIO()->find(mZBIO)!=mDico->ZBIO()->end();}
    bool hasTOPO(){ return mDico->topo()->find(mTOPO)!=mDico->topo()->end();}
    bool hasST(){ return mDico->station(mZBIO,mSt)!="not found";}
    bool readyFEE(){ return hasNH() && hasNT() && hasZBIO() && hasTOPO();}
    bool readyCS(){ return hasZBIO() && hasST();}
    bool hasEss(){ return HaveEss;}
    double getX(){return _x;}
    double getY(){return _y;}
    bool ecogramme(){return hasFEEApt;}
    bool isOK(){return !mEmpty;}

    int mNH,mNT,mZBIO,mTOPO;
    bool HaveEss;
    std::shared_ptr<cEss> mActiveEss; // l'essence qui intéresse l'utilisateur
    // catalogue de station
    int mSt;

    OGRPoint getPoint(){
        OGRPoint pt(_x,_y);
        return pt;}
private:
    bool mEmpty;
    bool hasFEEApt;// pour savoir si l'écogramme est dessiné ou pas
    double _x,_y;

};

class MyRenderer : public Wt::Render::WPdfRenderer {

public:

  MyRenderer(HPDF_Doc pdf, HPDF_Page page,std::string aTitle,cDicoApt * aDico): WPdfRenderer{pdf, page},pdf_{pdf},mytitle(aTitle),mDico(aDico)
  {
    setMargin(2.54, Wt::Side::Left | Wt::Side::Right | Wt::Side::Bottom);
    //setMargin(2.54, Wt::Side::Top);
    setMargin(2.54, Wt::Side::Top); // la mesure est en cm, donc j'ai laissé 1 centimètre de header
    setDpi(96);

    struct tm* to;
    time_t t;
    t = time(NULL);
    to = localtime(&t);
    strftime(datename, sizeof(datename), "%Y-%m-%d", to);

    // header sur page 1
    renderHeader(page,0);

  }

  HPDF_Page createPage ( int pageNum)
  {
    //setMargin(3.54, Wt::Side::Top);
    setMargin(2.54, Wt::Side::Top);
    HPDF_Page newPage = WPdfRenderer::createPage(pageNum);       // then just pass back to the original render::createpage


    renderHeader(newPage,pageNum);
    return newPage;   // And return as normal
  }

  void renderHeader(HPDF_Page page, int pageNum) {
    // je vais ajouter la bannière de Forestimator et la date en + du num de page
    WPdfRenderer headerFooterRenderer{pdf_, page};
    headerFooterRenderer.setMargin(1.54);
    setDpi(96);
   /* headerFooterRenderer.render(Wt::WString(
        "<p style=\"text-align: center; margin-top: 0;\">Header</p>"
        "<div style=\"height: 840px;\"></div>"
        "<p style=\"text-align: center; margin-bottom: 0;\">Footer</p>"));*/
    /*
    std::string baliseBannierePdf = Wt::WString::tr("report.ban").toUTF8();
    boost::replace_all(baliseBannierePdf,"PATH",mDico->File("docroot"));
    headerFooterRenderer.render(baliseBannierePdf);
      ça fonctionne mais c'est pas très beau... */

    WPdfRenderer r2{pdf_, page};
    r2.setMargin(1.54);
    r2.render(Wt::WString("<p style=\"text-align: center; margin-top: 0; font-size: 8pt;\"> Forestimator - "+mytitle+", "+  datename+", page "+ std::to_string(pageNum+1) + "</p>"));


  }

private:
  std::string mytitle;
  HPDF_Doc pdf_;
  cDicoApt * mDico;
  char datename[32];

};


class pointPdfResource : public Wt::WResource
{
public:

pointPdfResource(simplepoint * mSimplePoint) : WResource(), mSP(mSimplePoint){}

    ~pointPdfResource()
    {
        beingDeleted();
    }
void handleRequest(const Http::Request &request, Http::Response &response);


private:
simplepoint * mSP;

};

#endif // LEGEND_H
