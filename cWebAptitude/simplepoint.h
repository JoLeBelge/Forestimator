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
    void export2pdf();
    void export2pdf1();

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


#endif // LEGEND_H
