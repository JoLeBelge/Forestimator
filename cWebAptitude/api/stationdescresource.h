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


class stationDescResource : public Wt::WResource
{
public:
   stationDescResource(cDicoApt * adico):mDico(adico){}

   virtual void handleRequest(const Wt::Http::Request &request,
                              Wt::Http::Response &response) override
   {
     response.setMimeType("text/plain");

      /*response.out() << "Request path:\n"<< request.path() << "\n\n";

     auto pathInfo = request.pathInfo();
    if (pathInfo.empty())
       pathInfo = "(empty)";
     response.out() << "Request path info:\n"
                    << pathInfo << "\n\n";

     response.out() << "Request URL parameters\n"
                       "----------------------\n";
                       */

     auto params = request.urlParams();

     //if (params.empty()) response.out() << "(empty)\n";

     for (const auto &param : params) {

       const auto &name = param.first;
       const auto &value = param.second;
       if (name=="tool") {mParamTool=value;}
       if (name=="toolarg") {mParamArgs=value;}
       if (name=="pol") {mParamPolyg=value;}
       //response.out() << name << ": " << value << '\n';
     }

     //response.out() << "Réponse\n" <<"----------------------\n";

     response.out() << geoservice();
   }

   std::string geoservice();
   bool checkTool();
   bool checkPolyg();


private:
    cDicoApt * mDico;
    std::string mParamTool, mParamArgs,mParamPolyg;
    OGRGeometry * mPol;
};

#endif // STATIONDESCRESOURCE_H
