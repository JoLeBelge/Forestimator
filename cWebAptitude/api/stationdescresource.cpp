#include "stationdescresource.h"

std::string stationDescResource::geoservice(){
    std::string aRes("");
    // je suis pas dans une appli Wt donc je n'ai pas accès au docroot malheureusement...
    //Wt::WMessageResourceBundle msg();
    //msg.use(docRoot() + "/forestimator");
    GDALAllRegister();
    if (checkTool()){
        if (checkPolyg()){

            std::shared_ptr<layerBase> l=mDico->getLayerBase(mParamTool);
            // analyse surfacique ; basic stat pour les var continue
            switch (l->getTypeVar()) {
            case TypeVar::Continu:{
                std::cout << " api compute basic stat on polyg " << std::endl;
                basicStat stat=l->computeBasicStatOnPolyg(mPol);
                std::cout << " done " << std::endl;
                aRes+="mean;"+stat.getMean()+"\n";
                aRes+="max;"+stat.getMax()+"\n";
                aRes+="sd;"+stat.getSd()+"\n";
                break;

            }
            case TypeVar::Classe:{
                std::cout << " api compute valeur majoritaire " << std::endl;
                std::pair<int,double> p= l->valMajoritaire(mPol);
                aRes+="maj;"+std::to_string(p.first)+"\n";
                aRes+="pct;"+roundDouble(p.second,0)+"\n";

                break;
            }
            default:
                break;
            }

        } else { aRes="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370).";}//Wt::WText::tr("api.msg.error.polyg1").toUTF8();}
    } else {
        aRes="Aucune couche ou traitement de ce nom.";
    }


    return aRes;
}

bool stationDescResource::checkTool(){
    bool aRes(0);
    // pour bien faire, il faudrait également instancier une liste de "traitements" qui ne sont pas sensus stricto lié à une couche en particulier, exemple avec
    //1) compo ; plusieurs carte
    //2) aptitude des essences principales, plusieurs cartes également

    if (mDico->hasLayerBase(mParamTool)){ aRes=1;}
    return aRes;
}

bool stationDescResource::checkPolyg(){
    bool aRes(0);
    // lecture du polygone
    OGRSpatialReference src;
    /*src.importFromWkt()
     OGRErr err=src.SetWellKnownGeogCS( "EPSG:31370" );
     std::cout << "OGR error setwellKnownGeog : " << err << std::endl; // failure! je sais pas pk
    */
    OGRErr err=OGRGeometryFactory::createFromWkt(mParamPolyg.c_str(),NULL,&mPol);
    std::cout << "createFromWkt OGR error : " << err << std::endl;
    //std::cout << "src " << src.exportToProj4();
    if (err==OGRERR_NONE & mPol!=NULL & mPol->IsValid()){aRes=1;}
    return aRes;
}

