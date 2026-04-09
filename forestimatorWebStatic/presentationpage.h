#ifndef PRESENTATIONPAGE_H
#define PRESENTATIONPAGE_H
#include <Wt/WContainerWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WAnimation.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>
#include <Wt/WTemplate.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WSignal.h>
#include <Wt/WText.h>
#include <Wt/WTable.h>
#include <Wt/WTree.h>
#include <Wt/WTreeTable.h>
#include <Wt/WTreeTableNode.h>
#include <Wt/WCheckBox.h>
#include <Wt/WMessageBox.h>
#include <Wt/WLoadingIndicator.h>
#include <Wt/WPanel.h>
#include <Wt/WAnchor.h>
#include <fstream>
#include <Wt/WProgressBar.h>
#include <string.h>
#include <memory>
#include "cdicoapt.h"
#include <Wt/WVideo.h>
#include "mataptcs.h"

using namespace Wt;

class presentationPage : public Wt::WContainerWidget
{
public:
    presentationPage(cDicoApt *aDico, cWebAptitude *app);

private:
    cDicoApt *mDico;
    cWebAptitude *m_app;
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
