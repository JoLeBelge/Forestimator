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
#include <Wt/WEvent.h>
#include <Wt/WSignal.h>
#include <sys/stat.h>
#include <fstream>
#include <boost/algorithm/string/replace.hpp>
#include "cdicoapt.h"

#include "ogrsf_frmts.h"
#include "gdal_utils.h"

#include <ctime>

using namespace Wt;

class WOpenLayers: public WContainerWidget
{
public:
    WOpenLayers( cDicoApt * aDico);

    /*WOpenLayers(){
        printf("destructor wopenlayer\n");
        //mDico=NULL;
    }*/


    // pas simple d'impletemter ses signaux, voir https://redmine.webtoolkit.eu/boards/2/topics/12782?r=12807#message-12807

    void filterMouseEvent(WMouseEvent event){
        if (event.modifiers().test(Wt::KeyboardModifier::Shift)){
            // ici j'aimerai que le JS slot2 fonctionne et envoi sa réponse
            slot2.exec();
        }
    }

    void TouchStart(){
        timer = clock();
    }

    void TouchEnd(){
        clock_t touchLength = clock(); - timer;
        std::cout << " la durée du touch est de " << touchLength <<  std::endl;

        if (touchLength>1){
            slot.exec();
        }
    }

    virtual void layoutSizeChanged(int width, int height)
    {
        WContainerWidget::layoutSizeChanged(width, height);
        // Force a recalculation of the map viewport size. This should be called when third-party code changes the size of the map viewport
        doJavaScript("map.updateSize();");
    }
    cDicoApt * mDico;
    JSignal<double,double>& xy() { return xy_; }
    JSlot slot;
    JSignal<double, double>  xy_;

    JSignal<int>& polygId() { return polygId_; }
    JSlot slot2;

    JSignal<int>  polygId_;

    static constexpr const char *clickWithShift_label = "toto";

    clock_t timer;

};

#endif // WOPENLAYERS_H
