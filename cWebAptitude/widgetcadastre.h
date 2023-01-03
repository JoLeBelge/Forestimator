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
#include <Wt/WSignal.h>
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

    Wt::Signal<std::string, std::string>& sendPolygone() { return pathGeoJson_; }
    Wt::Signal<std::string, std::string> pathGeoJson_;

    // la session est créé ici car dépend de la session d'utilisateur, ne dois pas être propre au dicoApt ou à classe cadastre
    // pas sur que ce soit vraiment ideal comme ça. Je pense que je dois détourner l'usage premier du dbo de Wt qui fait du mapping orienté "session", alors que moi je veux du mapping
    // d'objet constant d'une session à l'autre, plutôt à mettre dans les dictionnaires utilisé par toutes les sessions.. voir ce que j'ai fait pour phytospy par ex.

    dbo::Session session;
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
    // clé ; index dans combobox. val= FID du polygone
    std::map<int,int> aMLabelPaCa;
};

#endif // WIDGETCADASTRE_H
