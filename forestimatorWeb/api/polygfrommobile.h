#ifndef POLYGFROMMOBILE_H
#define POLYGFROMMOBILE_H
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
using namespace Wt;

class polygFromMobile : public Wt::WResource
{
public:
    polygFromMobile(std::string aFileBD);
    ~polygFromMobile(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
private:
     dbo::Session session;
};

class validCompoRaster{
public:
    std::string  essence, rmq, nom_contact,contact, geom ;
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, essence,       "essence");
        dbo::field(a, rmq,   "rmq");
        dbo::field(a, nom_contact,    "nom_contact");
        dbo::field(a, contact,  "contact");
        dbo::field(a, geom,  "geom");
    }
};

#endif // POLYGFROMMOBILE_H
