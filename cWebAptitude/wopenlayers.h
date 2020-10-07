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

#include <chrono>
#include "layer.h"

using namespace std::chrono;

using namespace Wt;

class WOpenLayers: public WContainerWidget
{
public:
    WOpenLayers( cDicoApt * aDico);

    /*WOpenLayers(){
        printf("destructor wopenlayer\n");
        //mDico=NULL;
    }*/

    void updateView(){
        doJavaScript("activeLayer.getSource().changed();");
    }

    // pas simple d'impletemter ses signaux, voir https://redmine.webtoolkit.eu/boards/2/topics/12782?r=12807#message-12807

    void filterMouseEvent(WMouseEvent event){
        if (event.modifiers().test(Wt::KeyboardModifier::Shift)){
            // ici j'aimerai que le JS slot2 fonctionne et envoi sa réponse
            slot2.exec(std::to_string(event.widget().x),std::to_string(event.widget().y));
        }
    }

    void TouchStart(){
        timer = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    }
    void TouchMoved(){
        timer = duration_cast< milliseconds >(system_clock::now().time_since_epoch());
    }

    void TouchEnd(WTouchEvent t){
        milliseconds touchLength = duration_cast< milliseconds >(system_clock::now().time_since_epoch()) - timer;
        //std::cout << " la durée du touch est de " << touchLength.count() << std::endl;//<< " soit " << touchL << " seconde " << std::endl;
        if (touchLength.count()>100){
            //std::cout << "number of changedTouches " << t.changedTouches().size() << std::endl;
            if (t.changedTouches().size()>0){
            Wt::Touch touch = t.changedTouches()[0];
            //std::cout << " touch est de " << touch.screen().x << std::endl;
            slot3.exec(std::to_string(touch.widget().x),std::to_string(touch.widget().y));
            }
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
    // signaux pour selectionner un polygone du parcellaire au moment du shift+click
    // ol 4 gérais cet aspect bien mais sous le 6 il y a un bug à cause de la condition shiftOnly donc on migre tout vers c++
    JSignal<double, double>  xySelect_;
    JSignal<double,double>& xySelect() { return xySelect_; }

    JSlot slot2;

    //JSignal<int>& polygId() { return polygId_; }
    //JSignal<int>  polygId_;

    static constexpr const char *clickWithShift_label = "toto";

    //clock_t timer;
    milliseconds timer;

     JSlot slot3;

};

#endif // WOPENLAYERS_H
