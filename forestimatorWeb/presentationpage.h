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
    cDicoApt * mDico;
private:
    cWebAptitude* m_app;

};

#endif // PRESENTATIONPAGE_H
