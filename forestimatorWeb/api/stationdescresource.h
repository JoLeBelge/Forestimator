#ifndef STATIONDESCRESOURCE_H
#define STATIONDESCRESOURCE_H
#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <iostream>
#include "cdicoapt.h"
#include "layerbase.h"
#include <Wt/WFileUpload.h>

extern std::string nameDendroTool;
using namespace Wt;

// voir aussi te-benchmark/ example

// problème avec ces ressources dynamiques, c'est que je passe par des membres (mParamTool, mResponse) et ceux-ci gardent leurs états dune requête à lautre, dun utilisateur à lautre
// je peux réinitialiser la ressource lors d'une nouvelle requête mais je suis sur que j'aurai une merde si jamais 2 requêtes sont effectuées en même temps...
// solution; ne pas modifiers des variables membres durant le handlerequest. plus difficile à coder mais ça devrait fonctionner.

class stationDescResource : public Wt::WResource
{
public:
   stationDescResource(cDicoApt * adico):mDico(adico){}
   ~stationDescResource(){beingDeleted();}
   void handleRequest(const Http::Request &request,Http::Response &response);
private:
    cDicoApt * mDico;
};

#endif // STATIONDESCRESOURCE_H
