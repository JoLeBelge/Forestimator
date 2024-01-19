#ifndef ANAPONCTUELLERESOURCE_H
#define ANAPONCTUELLERESOURCE_H
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

class anaPonctuelleResource : public Wt::WResource
{
public:
    anaPonctuelleResource(cDicoApt * adico):mDico(adico){}
    ~anaPonctuelleResource(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
 private:
     cDicoApt * mDico;
};

#endif // ANAPONCTUELLERESOURCE_H
