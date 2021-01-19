#ifndef STATIONDESCRESOURCE_H
#define STATIONDESCRESOURCE_H
#include <Wt/WServer.h>
#include "Wt/WResource.h"
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>

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

     response.out() << "Request path:\n"
                    << request.path() << "\n\n";

     auto pathInfo = request.pathInfo();
     if (pathInfo.empty())
       pathInfo = "(empty)";
     response.out() << "Request path info:\n"
                    << pathInfo << "\n\n";

     response.out() << "Request URL parameters\n"
                       "----------------------\n";

     auto params = request.urlParams();

     if (params.empty())
       response.out() << "(empty)\n";

     for (const auto &param : params) {

       const auto &name = param.first;
       if (name=="tool") {mParamTool=param.second;}
       if (name=="toolarg") {mParamArgs=param.second;}
       if (name=="polygon") {mParamPolyg=param.second;}

       const auto &value = param.second;
       response.out() << name << ": " << value << '\n';
     }

     response.out() << "Réponse\n"
                       "----------------------\n";
     response.out << geoservice();
   }

   std::string geoservice();
   bool checkTool();
   bool checkPolyg();


private:
    cDicoApt * mDico;
    std::string mParamTool, mParamArgs,mParamPolyg;
};

#endif // STATIONDESCRESOURCE_H
