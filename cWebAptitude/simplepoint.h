#ifndef LEGEND_H
#define LEGEND_H
#include "grouplayers.h"
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WMenuItem.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPopupMenuItem.h>
#include <Wt/WNavigationBar.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WEnvironment.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WPopupMenu.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WLabel.h>
#include <Wt/WToolBar.h>
#include <Wt/WTemplate.h>
#include <Wt/WString.h>
#include <sys/stat.h>
#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include "ecogrammeEss.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <Wt/Render/WPdfRenderer.h>
#include "iostream"
#include <Wt/WRasterImage.h>

class simplepoint;
class groupLayers;
class ST;
class Layer;
using namespace Wt;
class color;
class MyRenderer; // dérivé de classe WPdfRenderer pour laquelle on ajoute des footers, voir https://redmine.webtoolkit.eu/boards/2/topics/14334

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
    simplepoint(groupLayers *aGL,WContainerWidget *parent);
    void createUI();
    void vider();
    void titreInfoRaster();
    void add1InfoRaster(std::vector<std::string> aV);
    void detailCalculAptFEE(ST *aST);
    //void afficheLegendeIndiv(const Layer *l);
    void afficheAptAllEss();
    void export2pdf(std::string titre);
    void export2pdfTitreDialog();

    Wt::WTable                 *mInfoT;
    Wt::WTable                 *mDetAptFEE;
    Wt::WTable                 *mAptAllEss;
    Wt::WContainerWidget     * mParent;
    WPushButton * createPdfBut;
    EcogrammeEss       *mEcoEss;
private:

    Wt::WContainerWidget     * mContEco;
    groupLayers*mGL;
    Wt::WText                  *titre_;
    cDicoApt * mDico;

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



#endif // LEGEND_H
