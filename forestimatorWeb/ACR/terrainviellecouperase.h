#ifndef TERRAINVIELLECOUPERASE_H
#define TERRAINVIELLECOUPERASE_H
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
    int AO_objectif, AO_travauxFutur, AO_travauxPasse, AO_retardIntervention ;
    string AO_hbille;

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
        dbo::field(a, AO_objectif,   "AO_objectif");
        dbo::field(a, AO_travauxPasse,   "AO_travauxPasse");
        dbo::field(a, AO_travauxFutur,   "AO_travauxFutur");
        dbo::field(a, AO_retardIntervention,   "AO_retardIntervention");
        dbo::field(a, AO_hbille,   "AO_hbille");
    }
};

class arbreGUI  : public std::enable_shared_from_this<arbreGUI>
{
public:

    dbo::Session * session;

    WTableRow * row;
    WLineEdit *type,*quadrat,*circ,*statut,*rege,*dist,*azim,*defaut,*rmq,*Ht;

    WLineEdit *objectif,*travauxPasse,*travauxFutur,*retardIntervention,*hbille;

    WLineEdit *ess;

    arbreGUI(dbo::Session * session,WTableRow * row):session(session),row(row){
        type =row->elementAt(0)->addNew<WLineEdit>();
        quadrat =row->elementAt(1)->addNew<WLineEdit>();
        ess =row->elementAt(2)->addNew<WLineEdit>();

        dist =row->elementAt(3)->addNew<WLineEdit>();
        circ =row->elementAt(4)->addNew<WLineEdit>();
        azim =row->elementAt(5)->addNew<WLineEdit>();
        Ht =row->elementAt(6)->addNew<WLineEdit>();
        statut =row->elementAt(7)->addNew<WLineEdit>();
        rege =row->elementAt(8)->addNew<WLineEdit>();
        defaut =row->elementAt(9)->addNew<WLineEdit>();
        rmq =row->elementAt(10)->addNew<WLineEdit>();
        objectif=row->elementAt(11)->addNew<WLineEdit>();
        travauxPasse=row->elementAt(12)->addNew<WLineEdit>();
        travauxFutur=row->elementAt(13)->addNew<WLineEdit>();
        retardIntervention=row->elementAt(14)->addNew<WLineEdit>();
        hbille=row->elementAt(15)->addNew<WLineEdit>();

        type->enterPressed().connect(quadrat, &WWidget::setFocus);
        quadrat->enterPressed().connect(ess, &WWidget::setFocus);
        ess->enterPressed().connect(dist, &WWidget::setFocus);
        dist->enterPressed().connect(circ, &WWidget::setFocus);
        circ->enterPressed().connect(azim, &WWidget::setFocus);
        azim->enterPressed().connect(Ht, &WWidget::setFocus);
        Ht->enterPressed().connect(statut, &WWidget::setFocus);
        statut->enterPressed().connect(rege, &WWidget::setFocus);
        rege->enterPressed().connect(defaut, &WWidget::setFocus);
        defaut->enterPressed().connect(rmq, &WWidget::setFocus);
        rmq->enterPressed().connect(objectif, &WWidget::setFocus);
        objectif->enterPressed().connect(travauxPasse, &WWidget::setFocus);
        travauxPasse->enterPressed().connect(travauxFutur, &WWidget::setFocus);
        travauxFutur->enterPressed().connect(retardIntervention, &WWidget::setFocus);
        retardIntervention->enterPressed().connect(hbille, &WWidget::setFocus);

        circ->setValidator(std::make_shared<WIntValidator>(0,1000));
        dist->setValidator(std::make_shared<WDoubleValidator>(0.0,11.0));
        Ht->setValidator(std::make_shared<WDoubleValidator>(0.0,50.0));
        statut->setValidator(std::make_shared<WIntValidator>(0,200));
        rege->setValidator(std::make_shared<WIntValidator>(0,20));
        azim->setValidator(std::make_shared<WIntValidator>(0,360));
        hbille->setValidator(std::make_shared<WDoubleValidator>(0.0,40.0));
        objectif->setValidator(std::make_shared<WIntValidator>(0,1000));
        travauxPasse->setValidator(std::make_shared<WIntValidator>(0,1000));
        travauxFutur->setValidator(std::make_shared<WIntValidator>(0,1000));
        retardIntervention->setValidator(std::make_shared<WIntValidator>(0,1000));

    }
    void add(int acr_id, int ue_id){

        if (dist->validate()==ValidationState::Valid && Ht->validate()==ValidationState::Valid){
            std::unique_ptr<arbre> tree = std::make_unique<arbre>();
            tree->ACR = acr_id;
            tree->UE = ue_id;
            tree->type = type->valueText().toUTF8();
            tree->quadrat = quadrat->valueText().toUTF8();
            tree->ess = ess->valueText().toUTF8();
            tree->Ht = Ht->valueText().toUTF8();
            tree->dist = dist->valueText().toUTF8();
            if(circ->valueText().toUTF8()!=""){
                tree->circ = std::stoi(circ->valueText().toUTF8());}
            if(statut->valueText().toUTF8()!=""){
                tree->statut = std::stoi(statut->valueText().toUTF8());}
            if(rege->valueText().toUTF8()!=""){
                tree->rege = std::stoi( rege->valueText().toUTF8());}
            if(azim->valueText().toUTF8()!=""){
                tree->azim = std::stoi( azim->valueText().toUTF8());}
            if(defaut->valueText().toUTF8()!=""){
                tree->defaut = std::stoi(defaut->valueText().toUTF8());}
            tree->rmq = rmq->valueText().toUTF8();
            if(defaut->valueText().toUTF8()!=""){
                tree->defaut = std::stoi(defaut->valueText().toUTF8());}
            if(objectif->valueText().toUTF8()!=""){
                tree->AO_objectif = std::stoi(objectif->valueText().toUTF8());}
            if(retardIntervention->valueText().toUTF8()!=""){
                tree->AO_retardIntervention = std::stoi(retardIntervention->valueText().toUTF8());}
            if(travauxFutur->valueText().toUTF8()!=""){
                tree->AO_travauxFutur = std::stoi(travauxFutur->valueText().toUTF8());}
            if(travauxPasse->valueText().toUTF8()!=""){
                tree->AO_travauxPasse = std::stoi(travauxPasse->valueText().toUTF8());}
            tree->AO_hbille= hbille->valueText().toUTF8();
            if (tree->ess!=""){
                //std::cout << "tree add()"<<std::endl;
                dbo::Transaction transaction(*session);
                dbo::ptr<arbre> treePtr = session->add(std::move(tree));
            }
        }
    }

    bool isValid(){
        return dist->validate()==ValidationState::Valid && Ht->validate()==ValidationState::Valid;
    }

    void clear(){
        ess->setText("");
        circ->setText("");
        statut->setText("");
        rege->setText("");
        dist->setText("");
        azim->setText("");
        defaut->setText("");
        Ht->setText("");
        rmq->setText("");
        hbille->setText("");
        objectif->setText("");
        travauxFutur->setText("");
        travauxPasse->setText("");
        retardIntervention->setText("");
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
        objectif->setText(b->objectif->valueText());
        retardIntervention->setText(b->retardIntervention->valueText());
        travauxFutur->setText(b->travauxFutur->valueText());
        travauxPasse->setText(b->travauxPasse->valueText());
        hbille->setText(b->hbille->valueText());
    }
};

class ue{
public:
    int id;
    int id_ACR;
    string bloquant,gps,rmq,rmqGPS, compo;
    int ori;
    int blocageSecteur1, blocageSecteur2;
    string gibierSecteur1, gibierSecteur2;
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, id_ACR,    "id_ACR");
        dbo::field(a, id,    "ue"); // faire attention à ne pas créer un champ nommé id car Wt utilise déjà un champs avec ce nom
        dbo::field(a, bloquant,   "bloquant");
        dbo::field(a, gps,   "gpsLabel");
        dbo::field(a, rmqGPS,   "rmqGPS");
        dbo::field(a, compo,   "compo");
        dbo::field(a, ori,   "ori");
        dbo::field(a, rmq,   "rmq");
        dbo::field(a, blocageSecteur1,   "blocageS1");
        dbo::field(a, blocageSecteur2,   "blocageS2");
        dbo::field(a, gibierSecteur1,   "gibierS1");
        dbo::field(a, gibierSecteur2,   "gibierS2");
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


class encodageRelTerrain : public Wt::WApplication
{
public:
    encodageRelTerrain(const Wt::WEnvironment& env, std::string aFileDB);
    WPushButton * submitUE,* loadUE, * saveACRbut;

    void saveACR();

    std::vector<std::unique_ptr<arbreGUI>> vArbres;
    void ajoutUE();
    void displayACR(int acr_id);

    Wt::WTable* tabNewEU, *tabAllEU;
    //UE
    Wt::WLineEdit * UE_id, * gpsLabel, *gpsRmq, * compo, *ueRmq, * ori, * gibierS1, *gibierS2, * blocageS1, * blocageS2;
    // ACR
    Wt::WLineEdit * ACR_id, * date, * ope, * gps;

    WTextArea * ACRrmq;

    WComboBox * comboA1, * comboA2;


private:
    dbo::Session session;
};


#endif // TERRAINVIELLECOUPERASE_H
