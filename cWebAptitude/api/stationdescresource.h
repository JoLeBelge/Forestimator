#ifndef STATIONDESCRESOURCE_H
#define STATIONDESCRESOURCE_H
#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
/*
#include <Wt/WText.h>
#include <Wt/WMessageResourceBundle.h>
*/
#include <iostream>

#include "cdicoapt.h"
#include "statHdomCompo.h"

// voir aussi te-benchmark/ example

// problème avec ces ressources dynamiques, c'est que je passe par des membres (mParamTool, mResponse) et ceux-ci gardent leurs états dune requête à lautre, dun utilisateur à lautre
// je peux réinitialiser la ressource lors d'une nouvelle requête mais je suis sur que j'aurai une merde si jamais 2 requêtes sont effectuées en même temps...
// solution 1; ne pas modifiers des variables membres durant le handlerequest. plus difficile à coder mais ça devrait fonctionner.
// solution 2 ; développer une application dédié au parsing des url api et création de WRessource liées à la session et non pas au serveur
// sol 1 implémenté avec succès

class stationDescResource : public Wt::WResource
{
public:
   stationDescResource(cDicoApt * adico):mDico(adico){}
   ~stationDescResource(){beingDeleted();}

   virtual void handleRequest(const Wt::Http::Request &request,
                              Wt::Http::Response &response) override
   {
      /* mParamTool="";
       mParamArgs="";
       mParamPolyg="";
       */
     //response.setMimeType("text/plain");
     //response.setMimeType("text/csv");
     //response.addHeader("charset","utf-8");
       // c'est la solution que j'ai trouvée pour que mes caractère accentués passent bien sur le navigateur
     response.addHeader("Content-Type","text/plain; charset=utf-8");


      /*response.out() << "Request path:\n"<< request.path() << "\n\n";

     auto pathInfo = request.pathInfo();
    if (pathInfo.empty())
       pathInfo = "(empty)";
     response.out() << "Request path info:\n"
                    << pathInfo << "\n\n";

     response.out() << "Request URL parameters\n"
                       "----------------------\n";
                       */

     if (request.path().substr(request.path().size()-4,4)=="help"){
         response.out() << "FORESTIMATOR API short help \n"
                           "---------------------------\n\n"
                           "\nListe des traitements pour analyse surfacique (analyse spécifique sur une couche ou analyse standard sur plusieurs couches) \n"
                           "----------------------------------------------------------------------------------------------------\n"
                           "hdom \n"
                           "compo\n"
                           "aptitude\n";

         response.out() <<  "Liste des couches accessibles via API\n"
                            "---------------------------\n";
         for (auto kv : mDico->VlayerBase()){
             if (kv.second->getCatLayer()!=TypeLayer::Externe){
             response.out() << kv.second->Code() + ", " + kv.second->Nom() + "\n";
             }
         }


     } else {

     auto params = request.urlParams();

     //if (params.empty()) response.out() << "(empty)\n";
     std::string aTool,aPolyg,aArgs;
     for (const auto &param : params) {

       const auto &name = param.first;
       const auto &value = param.second;
       if (name=="tool") {aTool=value;}
       if (name=="toolarg") {aArgs=value;}
       if (name=="pol") {aPolyg=value;}
       //response.out() << name << ": " << value << '\n';
     }

     //response.out() << "Réponse\n" <<"----------------------\n";

     response.out() << geoservice(aTool,aArgs,aPolyg);
     }
   }

   std::string geoservice(std::string aTool,std::string aArgs,std::string aPolyg);
   bool checkTool(std::string aTool);
   OGRGeometry * checkPolyg(std::string aPolyg);
   // pour commencer, uniquement une liste de code de carte d'aptitude (AG_FEE, ...)
   std::vector<std::string> parseAptArg(std::string aArgs);
   std::map<int,double> simplifieAptStat(std::map<int,double> aStat);
   std::vector<std::string> parseHdomArg(std::string aArgs);
   std::vector<std::string> parseCompoArg(std::string aArgs);

private:
    cDicoApt * mDico;
    /*std::string mParamTool, mParamArgs,mParamPolyg;
    std::string mResponse;
    OGRGeometry * mPol;*/
};

#endif // STATIONDESCRESOURCE_H
