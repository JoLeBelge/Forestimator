#include "stationdescresource.h"


extern bool globTest;

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
            if (name=="pts") {aPolyg=value;aMode=typeAna::MassPonctuel;}

        }

        OGRLayer * lay =NULL;
        if (aMode==typeAna::MassPonctuel){
            boost::filesystem::path tmpPath = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.geojson");
            std::ofstream ofs( tmpPath.c_str(), std::ofstream::out);
            ofs << request.in().rdbuf() ;
            ofs.close();
            GDALDataset * ds=static_cast<GDALDataset*>(GDALOpenEx(tmpPath.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, nullptr, nullptr, nullptr));
            if( ds == NULL ){
                response.out() << "geojson input not OK" ;
            } else{
                response.addHeader("Content-Type","application/json");
                lay = ds->GetLayer(0);
                mDico->geoservice(aTool,aArgs,aPolyg,aMode,lay);
                GDALClose(ds);
                std::ifstream inFile;
                inFile.open(tmpPath.c_str());
                response.out() << inFile.rdbuf();
                inFile.close();
            }
        } else {

        // si pas de polygone mais bien le nom d'une couche, on délivre le dictionnaire de la couche à l'utilisateur
        if (aPolyg==""){aMode=typeAna::dicoTable;
        }
         response.out() << mDico->geoservice(aTool,aArgs,aPolyg,aMode,lay);
        }



    }
}

