#ifndef RASTERCLIPRESOURCE_H
#define RASTERCLIPRESOURCE_H
#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <iostream>
#include "cdicoapt.h"
#include "layerbase.h"
#include <Wt/WFileUpload.h>
#include "gdal_utils.h"
using namespace Wt;

class rasterClipResource : public Wt::WResource
{
public:
    rasterClipResource(cDicoApt * adico):mDico(adico){}
    ~rasterClipResource(){beingDeleted();}
    void handleRequest(const Http::Request &request,Http::Response &response);
 private:
     cDicoApt * mDico;
};

#endif // RASTERCLIPRESOURCE_H
