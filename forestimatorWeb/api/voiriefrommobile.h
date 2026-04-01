#ifndef VOIRIEFROMMOBILE_H
#define VOIRIEFROMMOBILE_H

#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <iostream>
#include "cdicoapt.h"
#include "layerbase.h"
#include <Wt/WFileUpload.h>
#include <iostream>
#include <iomanip>
#include <Wt/WLocalDateTime.h>
using namespace Wt;

class voirieFromMobile : public Wt::WResource
{
public:
    voirieFromMobile(std::string aFileDB);
    ~voirieFromMobile(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
private:
     dbo::Session session;
};

class observationVoirie{
public:
    std::string  type,categorie, rmq, nom_contact,contact, date, geom ;
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, type,       "type");
        dbo::field(a, categorie,       "type");
        dbo::field(a, rmq,   "rmq");
        dbo::field(a, nom_contact,    "nom_contact");
        dbo::field(a, contact,  "contact");
        dbo::field(a, date,  "date");
        dbo::field(a, geom,  "geom");
    }
};

#endif // VOIRIEFROMMOBILE_H
