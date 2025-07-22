#include "anasurfresource.h"

void anaSurfResource::handleRequest(const Http::Request &request,Http::Response &response){

    auto params = request.urlParams();
    std::string listCode("");
    std::string aPolyg("");

    // "/api/anaSurf/layers/${listLayerCode}/polygon/${pol}"
    // "
    for (const auto &param : params) {
        const auto &name = param.first;
        const auto &value = param.second;
        if (name=="listLayerCode") {listCode=value;}
        if (name=="pol") {aPolyg=value;}
    }
    GDALAllRegister();

    std::vector<std::string> aV;
    boost::split( aV,listCode,boost::is_any_of("+"),boost::token_compress_on);

    OGRGeometry * pol=mDico->checkPolyg(aPolyg);
    if (pol!=NULL){

        response.addHeader("Content-Type","application/json");

        // le double to String de response.out m'arrondi le x (et pas le y, mystère). je fait le cast moi-même
        response.out() << "{\n\"forestimatorAnalyses\":\"surface\",\n"
                       <<       "\"surface\":"<< OGR_G_Area(pol)/10000 <<",\n"
                       <<       "\"RequestedLayers\":[\n";
        int j(0);
        for (std::string code: aV){

            response.out() << "    { \n\"layerCode\":\"" << code <<"\",\n";

            if (mDico->hasLayerBase(code)){

                std::shared_ptr<layerBase> l =mDico->getLayerBase(code);

                    switch (l->getTypeVar()) {
                    case TypeVar::Continu:{

                        basicStat stat=l->computeBasicStatOnPolyg(pol);
                        response.out() << "        \"mean\":"<< stat.getMean()<< ",\n"
                                       << "        \"max\":"<<  stat.getMax() << ",\n"
                                       << "        \"sum\":"<<  stat.getSum() << ",\n"
                                       << "        \"surf\":"<<  stat.getSurfTot(2) << ",\n"
                                       << "        \"surfNA\":"<<  stat.getNbNA(2) << ",\n"
                                       << "        \"sd\":" <<  stat.getSd() << "\n";
                        break;
                    }
                    case TypeVar::Classe:{
                        std::map<int,double> stat=l->computeStat2(pol);

                        if (l->getCatLayer()==TypeLayer::FEE){
                            stat=mDico->simplifieAptStat(stat);
                        }
                        response.out()     << "    \"classes\":[\n";
                        int c(0);
                        for (auto kv:stat){

                            response.out() << "        {\n"
                                           << "        \"rastValue\":"<<  kv.first<< ",\n"
                                           << "        \"value\":\""<<  l->getValLabel(kv.first) << "\",\n"
                                           << "        \"prop\":" <<  roundDouble(kv.second) << "\n"
                                           << "        }";
                            c++;
                            if (c<stat.size()) {response.out() << ",\n";}
                        }
                        response.out() << "\n      ]\n";
                        break;
                    }
                    default:
                        break;
                    }
                // fin if dico has layer
            }
            response.out()   << "    }";
            j++;
            if (j<aV.size()) {response.out() << ",\n";}
            // fin boucle sur layer
        }
        response.out() << "\n ]\n}\n";

        OGRGeometryFactory::destroyGeometry(pol);
    } else {
        response.out() <<"<error>La géométrie du polygone (ou du multipolygone) doit être valide et sa surface de maximum "+std::to_string(globMaxSurf)+"ha</error>";
    }

}
