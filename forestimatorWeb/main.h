#ifndef MAIN_H
#define MAIN_H

#include "cwebaptitude.h"
#include "Session.h"
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include "boost/program_options.hpp"
namespace po = boost::program_options;
#include <Wt/WServer.h>
#include <Wt/Auth/AuthWidget.h>
#include <Wt/Auth/PasswordService.h>
#include <Wt/WFileResource.h>
#include "api/stationdescresource.h"
#include "api/rasterclipresource.h"
#include "api/anaponctuelleresource.h"
#include "api/anasurfresource.h"
#include "analytics.h"
#include "./libzippp/src/libzippp.h"
using namespace libzippp;
#include "./threadpool/ThreadPool.hpp"
//#include "ACR/formviellecouperase.h"
#include "ACR/terrainviellecouperase.h"
//#include "desserteForest/desserteForest.h"
#include "api/staticmapresource.h"
#include "OGF/formOGF.h"

std::unique_ptr<Wt::WApplication> createWebAptitudeApplication(const Wt::WEnvironment &env, cDicoApt * dico);

int main(int argc, char **argv);

class layerResource : public Wt::WStreamResource
{
public:

layerResource(std::shared_ptr<layerBase> al,bool qml=0) : WStreamResource("plain/text"), ml(al),mQml(qml){}
    ~layerResource()
    {
        beingDeleted();
    }
void handleRequest(const Http::Request &request, Http::Response &response);


private:
std::shared_ptr<layerBase> ml;
bool mQml;

};

class ForestimatorMainTask : public Task {
    int *argc;
    char ***argv;
    void run() override;
public:
    ForestimatorMainTask(int *argc, char ***argv) : argc(argc), argv(argv){}
};


#endif // MAIN_H
