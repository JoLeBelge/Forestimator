#include "anasurfresource.h"

void anaSurfResource::handleRequest(const Http::Request &request,Http::Response &response){

    auto params = request.urlParams();
    std::string listCode("");
    std::string aPolyg("");

    // "/api/anaSurf/layers/${listLayerCode}/polygon/${pol}"
    for (const auto &param : params) {
        const auto &name = param.first;
        const auto &value = param.second;
        if (name=="listLayerCode") {listCode=value;}
        if (name=="polygon") {aPolyg=value;}
    }

    GDALAllRegister();

    std::vector<std::string> aV;
    boost::split( aV,listCode,boost::is_any_of("+"),boost::token_compress_on);

    OGRGeometry * pol=mDico->checkPolyg(aPolyg);
    if (pol!=NULL){

        response.addHeader("Content-Type","application/json");

        // le double to String de response.out m'arrondi le x (et pas le y, mystère). je fait le cast moi-même
        response.out() << "{\n\"forestimatorAnalyses\":\"surface\",\n"
                       <<       "\"RequestedLayers\":[\n";

        for (std::string code: aV){

            if (mDico->hasLayerBase(code)){

                std::shared_ptr<layerBase> l =mDico->getLayerBase(code);

                if(code=="CNSW"){
                    surfPedo surf(mDico->mPedo,pol);
                    response.out() << surf.getSummaryAPI() ;
                } else if (l->getCatLayer()==TypeLayer::FEE){
                    // response+="code_es;type;O;T;TE;E;I\n";

                    std::map<int,double> stat=l->computeStat2(pol);

                    //response+=l->EssCode()+";"+l->getCatLayerStr();
                    std::map<int,double> statSimp=mDico->simplifieAptStat(stat);
                    for (auto kv:statSimp){
                        //response+=";"+roundDouble(kv.second);
                    }
                    //response+="\n";
                    // carte dendro
                } else if (l->getCatLayer()==TypeLayer::CS){


                } else {


                    // analyse surfacique ; basic stat pour les var continue
                    switch (l->getTypeVar()) {
                    case TypeVar::Continu:{

                        basicStat stat=l->computeBasicStatOnPolyg(pol);


                        //  response+="mean;"+stat.getMean()+"\n";
                        //  response+="max;"+stat.getMax()+"\n";
                        //  response+="sd;"+stat.getSd()+"\n";

                        break;
                    }
                    case TypeVar::Classe:{
                        std::map<int,double> stat=l->computeStat2(pol);
                        // ici je met trois balise ; nom du field, valeur raster , et pourcentage
                        //     response+=putInBalise(l->getValLabel(kv.first),"classeName");
                        //     response+=putInBalise(std::to_string(kv.first),"classeRasterVal");
                        //     response+=putInBalise(roundDouble(kv.second),"pourcentage");

                        break;
                    }
                    default:
                        break;
                    }
                }

                // fin if dico has layer
            }
            // fin boucle sur layer
        }

        OGRGeometryFactory::destroyGeometry(pol);
    } else {
        response.out() <<"<error>La géométrie du polygone (ou du multipolygone) doit être valide et sa surface de maximum "+std::to_string(globMaxSurf)+"ha</error>";
    }

}
