#include "stationdescresource.h"


extern bool globTest;


std::string putInBalise(std::string aCont,std::string aBalise){

    return "<"+aBalise+">"+aCont+"</"+aBalise+">\n";

}

void stationDescResource::handleRequest(const Http::Request &request,Http::Response &response){

    if (request.path().substr(request.path().size()-4,4)=="help"){
        response.addHeader("Content-Type","text/plain; charset=utf-8");
        response.out() << "FORESTIMATOR API short help \n"
                          "---------------------------\n\n"
                          "\nListe des traitements pour analyse surfacique (analyse spécifique sur une couche ou analyse standard sur plusieurs couches) \n"
                          "----------------------------------------------------------------------------------------------------\n"
                          "hdom\n"
                          "aptitude\n"
                          "CNSW\n";

        response.out() <<  "Liste des couches accessibles via API et leur url WMS\n"
                           "---------------------------\n";
        for (auto kv : mDico->VlayerBase()){
            std::shared_ptr<layerBase> l=kv.second;
            if (l->getCatLayer()!=TypeLayer::Externe){
                response.out() << l->Code() + ", " + l->Nom() + " , "+ l->WMSURL() +" , layer " +l->WMSLayerName()+"\n";
            }
        }

    } else {

        auto params = request.urlParams();

        std::string aTool,aPolyg(""),aArgs;
        typeAna aMode(typeAna::surfacique);
        for (const auto &param : params) {

            const auto &name = param.first;
            const auto &value = param.second;
            if (name=="tool") {aTool=value;}
            if (name=="toolarg") {aArgs=value;}
            if (name=="pol") {aPolyg=value;}
            if (name=="pt") {aPolyg=value;aMode=typeAna::ponctuel;}
        }
        // si pas de polygone mais bien le nom d'une couche, on délivre le dictionnaire de la couche à l'utilisateur
        if (aPolyg==""){aMode=typeAna::dicoTable;
        }
        response.out() << mDico->geoservice(aTool,aArgs,aPolyg,aMode);
    }
}

