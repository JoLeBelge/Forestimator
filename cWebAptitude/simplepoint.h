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

  MyRenderer(HPDF_Doc pdf, HPDF_Page page,std::string aTitle): WPdfRenderer{pdf, page},pdf_{pdf},mytitle(aTitle)
  {
    setMargin(2.54, Wt::Side::Left | Wt::Side::Right | Wt::Side::Bottom);
    setMargin(2.54, Wt::Side::Top);
    //setMargin(3.54, Wt::Side::Top); // la mesure est en cm, donc j'ai laissé 1,5 centimètre de header
    setDpi(96);
    //renderHeader(currentPage()); pas de header sur page 1
  }

  HPDF_Page createPage ( int pageNum)
  {
    setMargin(3.54, Wt::Side::Top);
    HPDF_Page newPage = WPdfRenderer::createPage(pageNum);       // then just pass back to the original render::createpage
    renderHeader(newPage,pageNum);
    return newPage;   // And return as normal
  }

  void renderHeader(HPDF_Page page, int pageNum) {
    WPdfRenderer headerFooterRenderer{pdf_, page};
    headerFooterRenderer.setMargin(1.54);
    setDpi(96);
   /* headerFooterRenderer.render(Wt::WString(
        "<p style=\"text-align: center; margin-top: 0;\">Header</p>"
        "<div style=\"height: 840px;\"></div>"
        "<p style=\"text-align: center; margin-bottom: 0;\">Footer</p>"));*/
    headerFooterRenderer.render(Wt::WString("<p style=\"text-align: center; margin-top: 0; font-size: 8pt;\">"+mytitle+", page "+ std::to_string(pageNum+1) +"</p>"));
  }

private:
  std::string mytitle;
  HPDF_Doc pdf_;

};



#endif // LEGEND_H
