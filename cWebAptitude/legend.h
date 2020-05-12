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


class legend;
class groupLayers;
class ST;
class Layer;
using namespace Wt;
class color;

class legend: public WContainerWidget
{
public:
    legend(groupLayers *aGL,WContainerWidget *parent);//
    void createUI();
    void vider();
    void titreInfoRaster();
    void add1InfoRaster(std::vector<std::string> aV);
    void detailCalculAptFEE(ST *aST);
    void afficheLegendeIndiv(const Layer *l);
    void afficheAptAllEss();
    Wt::WTable                 *mInfoT;
    Wt::WTable                 *mDetAptFEE;
    Wt::WTable                 *mLegendIndiv;
    Wt::WTable                 *mAptAllEss;

    Wt::WContainerWidget     * mParent;
private:

    //Wt::WWidget     * mParent;
    groupLayers*mGL;
    Wt::WText                  *titre_;
    cDicoApt * mDico;

};

#endif // LEGEND_H
