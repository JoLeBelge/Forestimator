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

#include <Wt/Dbo/WtSqlTraits.h>

namespace dbo = Wt::Dbo;

//pour pouvoir classer les logs en différentes catégories. attention, ne pas changer l'ordre sinon la valeur de l'entier change et on est paumé
enum typeLog {unknown,page,extend,danap,anas,dsingle,dmulti};
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
    int             id_user; // optionnal
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
        case dsingle:
            aRes="télécharge une carte";
            break;
        case dmulti:
            aRes="télécharge plusieurs cartes";
            break;
        default:
            break;
        }
    }

    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, datum,    "datum");
        dbo::field(a, date,    "date");
        dbo::field(a, ip,       "ip");
        dbo::field(a, client,   "client");
        dbo::field(a, ipath,    "ipath");
        dbo::field(a, id_user,  "id_user");
        dbo::field(a, categorie,  "cat");
    }

};



class Analytics
{
public:
    Analytics(std::string aFileDB);
    //Analytics(const std::string &sqliteDb);
    bool logExist(const Wt::WEnvironment &env, std::string page, typeLog cat);

    void addLog(const Wt::WEnvironment &env, int user_id, std::string page,int cat=1);
    void addLog(const Wt::WEnvironment &env, int user_id){addLog(env,user_id,env.internalPath());}
    void addLog(const Wt::WEnvironment &env, std::string page,int cat=1){addLog(env,-1,page,cat);}
    void addLog(const Wt::WEnvironment &env){addLog(env,-1,env.internalPath());}

    dbo::Session session;
};

#endif // ANALYTICS_H

class PageAnalytics : public Wt::WApplication
{
public:
    PageAnalytics(const Wt::WEnvironment& env, std::string aFileDB);

    dbo::Session session;
};
