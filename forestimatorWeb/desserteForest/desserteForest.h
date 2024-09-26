#ifndef DF_H
#define DF_H
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WTableRow.h>
#include <Wt/WText.h>
#include <Wt/WTextArea.h>
#include <Wt/WLabel.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <string>
#include <iostream>
#include <fstream>
#include <Wt/WPushButton.h>
#include <Wt/WEvent.h>
#include <Wt/WSignal.h>
#include <Wt/WJavaScriptSlot.h>
#include <Wt/WMessageBox.h>
#include "Wt/WTemplate.h"
#include "layerstatchart.h"
#include <Wt/WItemDelegate.h>
#include <Wt/WStandardItem.h>
#include <Wt/WComboBox.h>
#include <Wt/Dbo/WtSqlTraits.h>
#include <Wt/WBootstrap5Theme.h>
#include <Wt/WLocalDateTime.h>
#include <cdicoapt.h>
#include <sqlite3.h>
#include <Wt/Mail/Message.h>
#include <Wt/Mail/Client.h>
#include "Wt/WDoubleValidator.h"
#include "Wt/WValidator.h"
#include <Wt/Dbo/Dbo.h>

// pour objet Wol
#include "ACR/formviellecouperase.h"

using namespace Wt;
namespace dbo = Wt::Dbo;

// pas sur que la classe soit nécéssaire vu que je n'utilise pas wt::dbo. Sauf pour créer la BD la première fois
class desserte{
public:
    std::string date,nom,prenom,mail,tel;
    int typeGeom;
    std::string typeContact,descr,geom,contactPrecision, typeAM,typeProprio,deposant;
    int id;
    template<class Action>
    void persist(Action& a)
    {
       // dbo::field(a, id,    "id");
        dbo::field(a, date,    "date");
        dbo::field(a, nom,   "nom");
        dbo::field(a, prenom,    "prenom");
        dbo::field(a, mail,  "mail");
        dbo::field(a, tel,  "tel");
        dbo::field(a, typeContact,  "typeContact");
        dbo::field(a, contactPrecision,  "contactPrecision");
        dbo::field(a, typeAM,  "typeAM");
        dbo::field(a, typeProprio,  "typeProprio");
        dbo::field(a, deposant,  "deposant");
        dbo::field(a, typeGeom,  "typeGeom");
        dbo::field(a, descr,  "descr");
        dbo::field(a, geom,  "geom");
    }
};

class formDesserteForest : public WApplication
{
public:
    formDesserteForest(const Wt::WEnvironment& env, cDicoApt * dico, std::string aFileDB);

    void loadStyles();
    void submit();
    void sendSummaryMail();
    void displayLayer(std::string aCode);
    void displayCommune();
    OGREnvelope computeGlobalGeom(std::string aFile);
    void validDraw(std::string geojson);
    void vider(bool all=1);
    std::string format4SQL(std::string aString);
    void changeGeom();

    WLineEdit *nom;
    WLineEdit *prenom;
    WLineEdit *mail;
    WLineEdit *tel;
    WComboBox *typeContact;
    WLineEdit *contactPresicion;

    WComboBox *typeAM;
    WComboBox *typeProprio;
    WComboBox *choixAM;
    WComboBox *deposant;
    WTextArea *description;

    WPushButton * bCancel;
    WComboBox * commune_;
    // clé ; index dans combobox. val= clé (INS commune , code Division)
    std::map<int,int> aMLabelCom;

private:

    std::string SQLstring;
    bool polygValid;
    std::string aGeom;
    OGRGeometry * geom;
    std::string mBDFile;
    dbo::Session session;

    cDicoApt * mDico;
};

#endif // DF_H
