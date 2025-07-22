#include "staticmapresource.h"
extern bool globTest;
void staticMapResource::handleRequest(const Http::Request &request,Http::Response &response){
        if (globTest) {std::cout << "staticMapResource:: handle request" << std::endl;}
        auto params = request.urlParams();
        std::string lCode(""),aPolyg(""),aEnv("");
        int aSz=500;

        for (const auto &param : params) {
            const auto &name = param.first;
            const auto &value = param.second;
            if (name=="layerCode") {lCode=value;}
            if (name=="pol") {aPolyg=value;}
            if (name=="env") {aEnv=value;}
            if (name=="sz") {aSz=std::stoi(value);}
        }
        GDALAllRegister();

        if (mDico->hasLayerBase(lCode)){
        std::shared_ptr<layerBase> l =mDico->getLayerBase(lCode);
        OGRGeometry * pol=mDico->checkPolyg(aPolyg);
        OGRGeometry * envGeom=mDico->checkPolyg(aEnv,10000);
         if (pol!=NULL){
         OGREnvelope * env= new OGREnvelope;
             if (envGeom!=NULL){
                 envGeom->getEnvelope(env);
             } else {
                 env=NULL;
             }

         staticMap sm(l,pol,env,aSz);
         std::ifstream r(sm.getFileName(), std::ios::in | std::ios::binary);
         response.addHeader("Content-Type","image/png");
         response.out() << r.rdbuf();
         r.close();
         }else {
             response.addHeader("Content-Type","text/plain; charset=utf-8");
             response.out() << "le polygone renseigné n'est pas valide" << std::endl;
         }
        }else {
            response.addHeader("Content-Type","text/plain; charset=utf-8");
            response.out() << "le code de couche carto renseigné n'est pas valide" << std::endl;
        }
}
