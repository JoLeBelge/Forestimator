#ifndef FORMVIELLECOUPERASE_H
#define FORMVIELLECOUPERASE_H
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
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



using namespace Wt;
namespace dbo = Wt::Dbo;

class acr{
public:
    std::string     date,vosRef,nom,prenom,contact,gsm;
    bool     keepInTouch;
    std::string typeContact,anneeCoupe,regeNat,vegeBloquante,objectif,spCoupe,sanitCoupe,travaux,plantation,gibier,descr,surf,polygon;
    int id;

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, id,    "id");
        dbo::field(a, date,    "date");
        dbo::field(a, vosRef,       "vosRef");
        dbo::field(a, nom,   "nom");
        dbo::field(a, prenom,    "prenom");
        dbo::field(a, contact,  "contact");
        dbo::field(a, typeContact,  "typeContact");
        dbo::field(a, anneeCoupe,  "anneeCoupe");
        dbo::field(a, regeNat,  "regeNat");
        dbo::field(a, typeContact,  "typeContact");
        dbo::field(a, anneeCoupe,  "anneeCoupe");
        dbo::field(a, regeNat,  "regeNat");
        dbo::field(a, vegeBloquante,  "vegeBloquante");
        dbo::field(a, objectif,  "objectif");
        dbo::field(a, spCoupe,  "spCoupe");
        dbo::field(a, sanitCoupe,  "sanitCoupe");
        dbo::field(a, travaux,  "travaux");
        dbo::field(a, plantation,  "plantation");
        dbo::field(a, gibier,  "gibier");
        dbo::field(a, descr,  "descr");
        dbo::field(a, surf,  "surf");
        dbo::field(a, polygon,  "polygon");
        dbo::field(a, keepInTouch,  "keepInTouch");
    }

};



class formVielleCoupeRase : public WApplication
{
public:
    formVielleCoupeRase(const Wt::WEnvironment& env, cDicoApt * dico, std::string aFileDB);

    void loadStyles();
    void submit();
    void sendSummaryMail();
    void displayLayer(std::string aCode);
    void displayCommune();
    OGREnvelope computeGlobalGeom(std::string aFile);
    void validDraw(std::string geojson);

    void vider(bool all=1);

    std::string format4SQL(std::string aString);

    WLineEdit *nomEncoderEdit_;
    WLineEdit *prenomEncoderEdit_;
    WLineEdit *contactEncoderEdit_;
    WLineEdit *contactEncoderGSMEdit_;
    WCheckBox * keepInTouch;
    WComboBox *typeContactEdit_;
    WComboBox *anneeVCREdit_;
    WLineEdit *regeNatEdit_;
    WLineEdit *vegeBloquanteEdit_;
    WTextArea *VCRdescriptionEdit_;

    WLineEdit *vosrefEdit_;
    WComboBox *objectifEdit_;
    WLineEdit *spEdit_;
    WLineEdit *sanitEdit_;
    WLineEdit *travSylviEdit_;
    WLineEdit *plantationEdit_;
    WLineEdit *gibierEdit_;

    WPushButton * bCancel;

    WComboBox * commune_;
    // clé ; index dans combobox. val= clé (INS commune , code Division)
    std::map<int,int> aMLabelCom;

private:

    std::string SQLstring;

    double surf;
    bool polygValid;
    std::string polyg;
    OGRGeometry * geom;
    std::string mBDFile;
    dbo::Session session;

    cDicoApt * mDico;
};

class Wol: public WContainerWidget
{
public:
    Wol();

    void updateView(){
        doJavaScript("refreshLayers();");
    }

    virtual void layoutSizeChanged(int width, int height)
    {
        WContainerWidget::layoutSizeChanged(width, height);
        doJavaScript("map.updateSize();");
    }

    JSignal<std::string>  polygGeojson_;
    JSignal<std::string>& polygGeojson() { return polygGeojson_; }
    JSlot slot;
};

class ACRAnalytics : public Wt::WApplication
{
public:
    ACRAnalytics(const Wt::WEnvironment& env, std::string aFileDB);
private:
      dbo::Session session;
};

#endif // FORMVIELLECOUPERASE_H
