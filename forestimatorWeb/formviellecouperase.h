#ifndef FORMVIELLECOUPERASE_H
#define FORMVIELLECOUPERASE_H
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

class arbre{
public:
    int ACR;
    int UE;
    //// soit Q1, Q2, Q3, Q4 ou alors Avenir (1 ou 2)
    std::string type;
    std::string quadrat;
    std::string ess;
    int statut;
    int rege;
    std::string Ht;
    int circ;
    std::string dist;
    int azim;
    int defaut;
    std::string rmq;

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, ACR,    "id_ACR");
        dbo::field(a, UE,    "UE");
        dbo::field(a, type,  "type");
        dbo::field(a, quadrat,  "quadrat");
        dbo::field(a, ess,       "ess");
        dbo::field(a, statut,    "statut");
        dbo::field(a, rege,       "rege");
        dbo::field(a, Ht,  "Ht");
        dbo::field(a, circ,    "circ");
        dbo::field(a, dist,  "dist");
        dbo::field(a, azim,  "azim");
        dbo::field(a, defaut,  "defaut");
        dbo::field(a, rmq,   "rmq");
    }
};

class arbreGUI  : public std::enable_shared_from_this<arbreGUI>
{
public:

    dbo::Session * session;

    WTableRow * row;
    WLineEdit *type,*quadrat,*circ,*statut,*rege,*dist,*azim,*defaut,*rmq,*Ht;

    WLineEdit *ess;

    arbreGUI(dbo::Session * session,WTableRow * row):session(session),row(row){
        type =row->elementAt(0)->addNew<WLineEdit>();
        quadrat =row->elementAt(1)->addNew<WLineEdit>();
        ess =row->elementAt(2)->addNew<WLineEdit>();

        //ess =row->elementAt(2)->addNew<WComboBox>();
        //typeContactEdit_->addItem(WString::tr("typeEncoder0"));

        circ =row->elementAt(3)->addNew<WLineEdit>();
        statut =row->elementAt(4)->addNew<WLineEdit>();
        rege =row->elementAt(5)->addNew<WLineEdit>();
        Ht =row->elementAt(6)->addNew<WLineEdit>();
        dist =row->elementAt(7)->addNew<WLineEdit>();
        azim =row->elementAt(8)->addNew<WLineEdit>();
        defaut =row->elementAt(9)->addNew<WLineEdit>();
        rmq =row->elementAt(10)->addNew<WLineEdit>();

        type->enterPressed().connect(quadrat, &WWidget::setFocus);
        quadrat->enterPressed().connect(ess, &WWidget::setFocus);
        ess->enterPressed().connect(circ, &WWidget::setFocus);
        circ->enterPressed().connect(statut, &WWidget::setFocus);
        statut->enterPressed().connect(rege, &WWidget::setFocus);
        rege->enterPressed().connect(Ht, &WWidget::setFocus);
        Ht->enterPressed().connect(dist, &WWidget::setFocus);
        dist->enterPressed().connect(azim, &WWidget::setFocus);
        azim->enterPressed().connect(defaut, &WWidget::setFocus);
        defaut->enterPressed().connect(rmq, &WWidget::setFocus);

        circ->setValidator(std::make_shared<WIntValidator>(0,1000));
        dist->setValidator(std::make_shared<WDoubleValidator>(0.0,11.0));
        Ht->setValidator(std::make_shared<WDoubleValidator>(0.0,50.0));
        statut->setValidator(std::make_shared<WIntValidator>(0,20));
        rege->setValidator(std::make_shared<WIntValidator>(0,20));
        azim->setValidator(std::make_shared<WIntValidator>(0,360));

    }
    void add(int acr_id, int ue_id){

        //if (dist->validate()==ValidationState::Valid && Ht->validate()==ValidationState::Valid){
            //std::cout << "tree add()"<<std::endl;

            std::unique_ptr<arbre> tree = std::make_unique<arbre>();
            tree->ACR = acr_id;
            tree->UE = ue_id;
            tree->type = type->valueText().toUTF8();
            tree->quadrat = quadrat->valueText().toUTF8();
            tree->ess = ess->valueText().toUTF8();

            //if(Ht->valueText().toUTF8()!=""){
            //std::cout << "Ht to double " ;
            tree->Ht = Ht->valueText().toUTF8();//std::stod(Ht->valueText().toUTF8());
            //std::cout << "done" << std::endl;
            //}
            //if(dist->valueText().toUTF8()!=""){
            //std::cout << "dist to double " ;
            tree->dist = dist->valueText().toUTF8();//std::stod( dist->valueText().toUTF8());
            //std::cout << "done" << std::endl;
            //}
            if(circ->valueText().toUTF8()!=""){tree->circ = std::stoi(statut->valueText().toUTF8());}
            if(statut->valueText().toUTF8()!=""){tree->statut = std::stoi(statut->valueText().toUTF8());}
            if(rege->valueText().toUTF8()!=""){tree->rege = std::stoi( rege->valueText().toUTF8());}
            if(azim->valueText().toUTF8()!=""){tree->azim = std::stoi( azim->valueText().toUTF8());}
            if(defaut->valueText().toUTF8()!=""){tree->defaut = std::stoi(defaut->valueText().toUTF8());}
            tree->rmq = rmq->valueText().toUTF8();
            if (tree->ess!=""){
                dbo::Transaction transaction(*session);
                dbo::ptr<arbre> treePtr = session->add(std::move(tree));
            }
        //}
    }

    bool isValid(){
        return dist->validate()==ValidationState::Valid && Ht->validate()==ValidationState::Valid;
    }

    void clear(){
        //std::cout << "tree clear()"<<std::endl;
        ess->setText("");
        circ->setText("");
        statut->setText("");
        rege->setText("");
        dist->setText("");
        azim->setText("");
        defaut->setText("");
        Ht->setText("");
        rmq->setText("");
    }

    void setLike(arbreGUI* b){
        ess->setText(b->ess->valueText());
        circ->setText(b->circ->valueText());
        statut->setText(b->statut->valueText());
        rege->setText(b->rege->valueText());
        dist->setText(b->dist->valueText());
        azim->setText(b->azim->valueText());
        defaut->setText(b->defaut->valueText());
        Ht->setText(b->Ht->valueText());
    }
};

class ue{
public:
    int id;
    int id_ACR;
    string bloquant,gps,rmq,rmqGPS, compo;
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, id_ACR,    "id_ACR");
        dbo::field(a, id,    "ue");
        dbo::field(a, bloquant,   "bloquant");
        dbo::field(a, gps,   "gpsLabel");
        dbo::field(a, rmqGPS,   "rmqGPS");
        dbo::field(a, compo,   "compo");
        dbo::field(a, rmq,   "rmq");
    }
};

class ACR_ter{
public:
    int id_ACR;
    string date,ope,gps,rmq;
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, id_ACR,    "id_ACR");
        dbo::field(a, date,  "date");
        dbo::field(a, ope,   "ope");
        dbo::field(a, gps,   "gps");
        dbo::field(a, rmq,   "rmq");
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

class encodageRelTerrain : public Wt::WApplication
{
public:
    encodageRelTerrain(const Wt::WEnvironment& env, std::string aFileDB);
    WPushButton * submitUE,* loadUE;

    std::vector<std::unique_ptr<arbreGUI>> vArbres;
    void ajoutUE();
    void displayACR(int acr_id);

    Wt::WTable* tabNewEU, *tabAllEU;
    Wt::WLineEdit * ACR_id, * UE_id, * gpsLabel, *gpsRmq, * compo, *ueRmq;


private:
    dbo::Session session;
};

#endif // FORMVIELLECOUPERASE_H
