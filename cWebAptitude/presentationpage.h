#ifndef PRESENTATIONPAGE_H
#define PRESENTATIONPAGE_H
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WAnimation.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>
#include <Wt/WTemplate.h>
#include "cdicoapt.h"
#include "grouplayers.h"

using namespace Wt;

class presentationPage : public Wt::WContainerWidget
{
public:
    presentationPage(cDicoApt * aDico);
    cDicoApt * mDico;
    //void bannerAnimation();
    //Wt::WStackedWidget * adsBanner;
};

#endif // PRESENTATIONPAGE_H
