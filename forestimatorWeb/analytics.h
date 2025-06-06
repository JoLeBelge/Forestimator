#ifndef ANALYTICS_H
#define ANALYTICS_H

#pragma once
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/Session.h>
#include <Wt/Dbo/ptr.h>
#include "Wt/Dbo/backend/Sqlite3.h"

#include <Wt/WDateTime.h>
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>

#include <Wt/Chart/WCartesianChart.h>
#include <Wt/WItemDelegate.h>
#include <Wt/Chart/WDataSeries.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WComboBox.h>

#include <Wt/Dbo/WtSqlTraits.h>
//#include <Wt/Date/date.h>

namespace dbo = Wt::Dbo;
using namespace Wt;
//using namespace date;

//pour pouvoir classer les logs en différentes catégories. attention, ne pas changer l'ordre sinon la valeur de l'entier change et on est paumé
enum typeLog {unknown,page,extend,danap,anas,dsingle,dmulti,danas,dsingleRW,selectLayer,rechercheCadastre,anasMulti};
// danap download pdf analyse ponctuelle.
// anas analyse surfacique
// dsingle download une carte
// dmulti download plusieurs cartes



class Log{
public:
    long            datum;
    Wt::WDate   date; // pas l'heure , mais le jour oui. si je veux l'heure ; Wt::WDateTime
    std::string     ip; // clientAddress()
    std::string     client; // userAgent()
    std::string     ipath; // internalPath() ou un message de description du log
   // int             id_user; // optionnal
    int16_t             client_id;
    int categorie;

    std::string getCat() const{
        std::string aRes("/");
        switch (categorie) {
        case page:
            aRes="page";
            break;
        case extend:
            aRes="extend";
            break;
        case danap:
            aRes="télécharge rapport analyse ponctuelle";
            break;
        case anas:
            aRes="analyse surfacique";
            break;
        case danas:
            aRes="télécharge rapport analyse surfacique";
            break;
        case anasMulti:
            aRes="analyses surfaciques sur un shp";
            break;
        case dsingle:
            aRes="télécharge une carte";
            break;
        case dsingleRW:
            aRes="télécharge une carte entière";
            break;
        case dmulti:
            aRes="télécharge plusieurs cartes";
            break;
        case rechercheCadastre:
            aRes="recherche Cadastrale";
            break;
        default:
            break;
        }
        return aRes;
    }

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, datum,    "datum");
        dbo::field(a, date,    "date");
        dbo::field(a, ip,       "ip");
        dbo::field(a, client,   "client");
        dbo::field(a, ipath,    "ipath");
       // dbo::field(a, id_user,  "id_user");
        dbo::field(a, client_id,  "client_id");
        dbo::field(a, categorie,  "cat");
    }

};



class Analytics
{
public:
    Analytics(std::string aFileDB);
    bool logExist(const Wt::WEnvironment &env, std::string page, typeLog cat);
    void addLogApache(const Wt::WEnvironment &env, std::string page);
    void addLog(const Wt::WEnvironment &env, std::string page,int cat=1);
    void addLog(const Wt::WEnvironment &env){addLog(env,env.internalPath());}
    dbo::Session session;
};

#endif // ANALYTICS_H

class PageAnalytics : public Wt::WApplication
{
public:
    PageAnalytics(const Wt::WEnvironment& env, std::string aFileDB);

    void setChart(int nbMonth);
    void changeGraph(){
        int nbm=std::stoi(nbMonthSelection_->currentText());
        setChart(nbm);
    }


    std::string getCat(int cat) {
        std::string aRes("/");
        switch (cat) {
        case 1:
            aRes="page";
            break;
        case 2:
            aRes="extend";
            break;
        case 3:
            aRes="télécharge rapport analyse ponctuelle";
            break;
        case 4:
            aRes="analyse surfacique";
            break;
        case 7:
            aRes="télécharge rapport analyse surfacique";
            break;
        case 5:
            aRes="télécharge une carte";
            break;
        case 8:
            aRes="télécharge une carte entière";
            break;
        case 6:
            aRes="télécharge plusieurs cartes";
            break;
        case 9:
            aRes="afficher une carte";
            break;
        default:
            break;
        }
        return aRes;
    }

private:

    dbo::Session session;
    Chart::WCartesianChart *mChart;
    WComboBox * nbMonthSelection_;
    std::shared_ptr<WStandardItemModel> model;
};
