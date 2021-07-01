#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/Session.h>
#include <Wt/Dbo/ptr.h>
#include "Wt/Dbo/backend/Sqlite3.h"

#include <Wt/WDateTime.h>
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <string>

namespace dbo = Wt::Dbo;

class Log{
public:
    long            datum;
    std::string     ip; // clientAddress()
    std::string     client; // userAgent()
    std::string     ipath; // internalPath()
    int             id_user; // optionnal


    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, datum,    "datum");
        dbo::field(a, ip,       "ip");
        dbo::field(a, client,   "client");
        dbo::field(a, ipath,    "ipath");
        dbo::field(a, id_user,  "id_user");
    }

};





class Analytics
{
public:
    Analytics(std::string aFileDB);
    //Analytics(const std::string &sqliteDb);

    void addLog(const Wt::WEnvironment &env, int user_id, std::string page);
    void addLog(const Wt::WEnvironment &env, int user_id){addLog(env,user_id,env.internalPath());}
    void addLog(const Wt::WEnvironment &env, std::string page){addLog(env,-1,page);}
    void addLog(const Wt::WEnvironment &env){addLog(env,-1,env.internalPath());}

    dbo::Session session;
};

#endif // ANALYTICS_H
