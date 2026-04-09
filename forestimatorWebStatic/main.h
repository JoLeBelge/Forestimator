#ifndef MAIN_H
#define MAIN_H

#include "cwebaptitude.h"
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include "boost/program_options.hpp"
namespace po = boost::program_options;
#include <Wt/WServer.h>
#include <Wt/Auth/AuthWidget.h>
#include <Wt/Auth/PasswordService.h>
#include <Wt/WFileResource.h>
#include "ACR/terrainviellecouperase.h"

std::unique_ptr<Wt::WApplication> createWebAptitudeApplication(const Wt::WEnvironment &env, cDicoApt *dico);

int main(int argc, char **argv);

class layerResource : public Wt::WStreamResource
{
public:
    layerResource(std::shared_ptr<layerBase> al, cDicoApt *aDico, int modeQml_Dico = 0) : WStreamResource("plain/text"), ml(al), mQmlDico(modeQml_Dico), mDico(aDico) {}
    ~layerResource()
    {
        beingDeleted();
    }
    void handleRequest(const Http::Request &request, Http::Response &response);

private:
    std::shared_ptr<layerBase> ml;
    int mQmlDico;
    cDicoApt *mDico;
};

#endif // MAIN_H
