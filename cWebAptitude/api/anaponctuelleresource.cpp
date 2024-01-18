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
    GDALAllRegister();

    //response.out() << " "
    std::vector<std::string> aV;
    boost::split( aV,listCode,boost::is_any_of("+"),boost::token_compress_on);
    response.addHeader("Content-Type","application/json");
    for (std::string code: aV){
        if (mDico->hasLayerBase(code)){
             std::shared_ptr<rasterFiles> l =mDico->getLayerBase(code);

             response.out() << "{ \n" << "'layerCode':'" << code <<"',\n"
                            << "'value':"<< l->getValue(x,y) << "\n"
                            << "}\n";
        }
    }

}
