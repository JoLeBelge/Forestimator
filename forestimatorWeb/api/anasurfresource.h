#ifndef ANASURFRESOURCE_H
#define ANASURFRESOURCE_H
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

class anaSurfResource: public Wt::WResource
{
public:
    anaSurfResource(cDicoApt * adico):mDico(adico){}
    ~anaSurfResource(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
 private:
     cDicoApt * mDico;
};

#endif // ANASURFRESOURCE_H
