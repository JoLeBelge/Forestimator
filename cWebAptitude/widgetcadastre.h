#ifndef WIDGETCADASTRE_H
#define WIDGETCADASTRE_H
#include "cadastre.h"
#include "Wt/WTable.h"
#include <Wt/WBreak.h>
#include <Wt/WSelectionBox.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLabel.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <vector>
#include <algorithm>

using namespace Wt;

class cadastre;

namespace Wt {
  class WContainerWidget;
  class WText;
  class WTextArea;
  class WComboBox;
  class WFormWidget;
}

class widgetCadastre : public WTable
{
public:
    widgetCadastre(cadastre * aCad);

    void refreshDivision();
    void refreshSection();
    void refreshPaCa();

    void submit();
private:
    cadastre * mCad;

    //WSelectionBox *n_;
    WComboBox * commune_;
    WComboBox * division_;
    WComboBox * section_;
    WComboBox * paCa_;
    WPushButton *submit_;

    // clé ; index dans combobox. val= clé (INS commune , code Division)
    std::map<int,int> aMLabelCom;
    std::map<int,int> aMLabelDiv;
    //std::map<int,int> aMLabelSec;
    std::map<int,int> aMLabelPaCa;
};

#endif // WIDGETCADASTRE_H
