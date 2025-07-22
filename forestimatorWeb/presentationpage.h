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
#include <Wt/WVideo.h>
#include "mataptcs.h"

using namespace Wt;

class presentationPage : public Wt::WContainerWidget
{
public:
    presentationPage(cDicoApt * aDico,cWebAptitude* app);

private:
    cDicoApt * mDico;
    cWebAptitude* m_app;
    Wt::WStackedWidget *contentsStack_;
    Wt::WPushButton *openMenuButton_;
    bool menuOpen_;
    void toggleMenu();
    void openMenu();
    void closeMenu();

    std::unique_ptr<WMenuItem> downloadPage();
    std::unique_ptr<WMenuItem> scolytePage();

};

#endif // PRESENTATIONPAGE_H
