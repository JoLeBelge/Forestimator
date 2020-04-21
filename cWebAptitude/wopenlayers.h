#ifndef WOPENLAYERS_H
#define WOPENLAYERS_H
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
#include "cdicoapt.h"

using namespace Wt;

class WOpenLayers: public WContainerWidget
{
public:
    WOpenLayers( cDicoApt * aDico);

    void giveFocus(bool b){
        // fonctionne pas pour le moment
        //std::cout << "\n\n\n focus pour la carte " << std::endl;
        setFocus(b);
    }

    cDicoApt * mDico;
    JSignal<double,double>& xy() { return xy_; }
    JSlot slot;
    void setJS_click();
    JSignal<double, double>  xy_;
};

#endif // WOPENLAYERS_H
