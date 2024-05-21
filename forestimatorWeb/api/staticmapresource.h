#ifndef STATICMAPRESOURCE_H
#define STATICMAPRESOURCE_H
#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <iostream>
#include "cdicoapt.h"
#include "layerbase.h"
#include <Wt/WFileUpload.h>
#include "layerstatchart.h"
using namespace Wt;

class staticMapResource : public Wt::WResource
{
public:
    staticMapResource(cDicoApt * adico):mDico(adico){}
    ~staticMapResource(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
private:
    cDicoApt * mDico;
};

#endif // STATICMAPRESOURCE_H
