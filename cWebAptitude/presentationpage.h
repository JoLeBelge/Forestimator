#ifndef PRESENTATIONPAGE_H
#define PRESENTATIONPAGE_H
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WAnimation.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>

using namespace Wt;

class presentationPage : public Wt::WStackedWidget
{
public:
    presentationPage();

    void bannerAnimation();
};

#endif // PRESENTATIONPAGE_H
