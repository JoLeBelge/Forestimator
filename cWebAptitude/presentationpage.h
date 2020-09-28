#ifndef PRESENTATIONPAGE_H
#define PRESENTATIONPAGE_H
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WAnimation.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>
#include <Wt/WTemplate.h>

using namespace Wt;

class presentationPage : public Wt::WContainerWidget
{
public:
    presentationPage();

    void bannerAnimation();
    Wt::WStackedWidget * adsBanner;
};

#endif // PRESENTATIONPAGE_H
