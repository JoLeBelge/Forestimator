#include "anaponctuelleresource.h"

void anaPonctuelleResource::handleRequest(const Http::Request &request,Http::Response &response){

    auto params = request.urlParams();
    std::string listCode("");
    double x,y;

    // "/api/anaPt/${listLayerCode}/x/${x}/y/${y}"
    for (const auto &param : params) {
        const auto &name = param.first;
        const auto &value = param.second;

        if (name=="listLayerCode") {listCode=value;}
        if (name=="x") {x=std::stod(value);}
        if (name=="y") {y=std::stod(value);}
    }
    /*std::cout << std::fixed;
    std::cout << std::setprecision(2);
    std::cout << "x " << x << ", y" << y <<std::endl;*/
    GDALAllRegister();

    // en amont du résultat pour les couches, ajouter des métadonnées, comme le fait qu\"on est bien en RW? le src du pt?
    bool inRW=mDico->getLayerBase("ZBIO")->getValue(x,y)!=0;

    // le double to String de response.out m'arrondi le x (et pas le y, mystère). je fait le cast moi-même
    response.out() << "{\n\"forestimatorAnalyses\":\"point\",\n"
                   <<       "\"x\":" << roundDouble(x) << ",\n"
                   <<       "\"y\":" <<  roundDouble(y) << ",\n"
                   <<       "\"src\":\"EPSG31370\",\n"
                   <<       "\"inRW\":" << inRW << ",\n"
                   <<       "\"RequestedLayers\":[\n";

    std::vector<std::string> aV;
    boost::split( aV,listCode,boost::is_any_of("+"),boost::token_compress_on);
    response.addHeader("Content-Type","application/json");
    int c(0);
    for (std::string code: aV){
        if (mDico->hasLayerBase(code)){
             std::shared_ptr<layerBase> l =mDico->getLayerBase(code);
             int v=l->getValue(x,y);
             std::string foundR=l->rasterExist() ? "true" : "false";
             response.out() << "    { \n\"layerCode\":\"" << code <<"\",\n"
                            << "    \"foundLayer\":true,\n"
                            << "    \"foundRastFile\":"<< foundR<< ",\n"
                            << "    \"rastValue\":"<<  v<< ",\n"
                            << "    \"value\":\""<<  l->getValLabel(v) << "\"\n"
                            << "    }";
        } else {
            response.out() << "    { \n" << "\"layerCode\":\"" << code <<"\",\n"
                           << "    \"foundLayer\":false,\n"
                           << "    \"foundRastFile\":false,\n"
                           << "    \"rastValue\":0,\n"
                           << "    \"value\":\"\"\n"
                           << "    }";
        }
        c++;
        if (c<aV.size()) { response.out() << ",\n";}
    }
    response.out() << "\n ]\n}\n";
}
