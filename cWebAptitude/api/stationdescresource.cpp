#include "stationdescresource.h"

int globMaxSurf(200);

std::string stationDescResource::geoservice(std::string aTool,std::string aArgs,std::string aPolyg){

    // je suis pas dans une appli Wt donc je n'ai pas accès au docroot malheureusement...
    //Wt::WMessageResourceBundle msg();
    //msg.use(docRoot() + "/forestimator");
    std::string aResponse;

    GDALAllRegister();
    if (checkTool(aTool)){
        OGRGeometry * pol=checkPolyg(aPolyg);
        if (pol!=NULL){
            if (aTool=="hdom"){
                std::vector<std::string> VMNH=parseHdomArg(aArgs);
                if (VMNH.size()==0){aResponse="arguments pour traitement 'hdom' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste de code de couche MNH séparées par une virgule\n";
                }else{
                    aResponse+="code_mnh;moy;cv;max;min;nb\n";
                    for (std::string code : VMNH){
                        std::shared_ptr<layerBase> l=mDico->getLayerBase(code);
                        statHdom stat(l,pol);
                        basicStat bs= stat.bshdom();
                        aResponse+=l->Code()+";"+bs.getMean()+";"+bs.getCV()+";"+bs.getMax()+";"+bs.getMin()+";"+bs.getNb()+"\n";
                    }
                }
            }else if (aTool=="compo"){
                std::vector<std::string> VCOMPO=parseCompoArg(aArgs);
                if (VCOMPO.size()==0){aResponse="arguments pour traitement 'compo' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste de code de couche de probabilité de présence d'une essence forestière, séparées par une virgule\n";
                }else{
                    std::vector<std::shared_ptr<layerBase>> Vlay;
                    for (std::string code : VCOMPO){
                        Vlay.push_back(mDico->getLayerBase(code));
                    }
                    statCompo stat(Vlay,mDico,pol);
                    aResponse+=stat.getAPIresult();
                }

            }else if(aTool=="aptitude"){
                std::vector<std::string> VCApt=parseAptArg(aArgs);
                aResponse+="code_es;type;O;T;TE;E;I\n";
                for (std::string code : VCApt){
                    std::shared_ptr<layerBase> l=mDico->getLayerBase(code);
                    std::map<int,double> stat=l->computeStat2(pol);
                    aResponse+=l->EssCode()+";"+l->getCatLayerStr();
                    std::map<int,double> statSimp=simplifieAptStat(stat);
                    for (auto kv:statSimp){
                        aResponse+=";"+roundDouble(kv.second);
                    }
                    aResponse+="\n";
                }

            } else {

                std::shared_ptr<layerBase> l=mDico->getLayerBase(aTool);
                // analyse surfacique ; basic stat pour les var continue
                switch (l->getTypeVar()) {
                case TypeVar::Continu:{
                    std::cout << " api compute basic stat on polyg " << std::endl;
                    basicStat stat=l->computeBasicStatOnPolyg(pol);
                    std::cout << " done " << std::endl;
                    aResponse+="mean;"+stat.getMean()+"\n";
                    aResponse+="max;"+stat.getMax()+"\n";
                    aResponse+="sd;"+stat.getSd()+"\n";
                    break;
                }
                case TypeVar::Classe:{
                    /*std::cout << " api compute valeur majoritaire " << std::endl;
                std::pair<int,double> p= l->valMajoritaire(pol);
                aResponse+="maj;"+std::to_string(p.first)+"\n";
                aResponse+="pct;"+roundDouble(p.second,0)+"\n";
                */
                    std::map<int,double> stat=l->computeStat2(pol);
                    std::string aL1,aL2;
                    bool test(1);
                    for (auto kv:stat){
                        if (test){
                            aL2=std::to_string(kv.first);
                            aL1=roundDouble(kv.second);
                            test=0;
                        }else{
                            aL2+=";"+std::to_string(kv.first);
                            aL1+=";"+roundDouble(kv.second);
                        }
                    }
                    aResponse+=aL2+"\n"+aL1+"\n";

                    break;
                }
                default:
                    break;
                }
            }
            OGRGeometryFactory::destroyGeometry(pol);
        } else {aResponse="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370). La géométrie du polygone doit être valide et sa surface de maximum "+std::to_string(globMaxSurf)+"ha";}
    } else {aResponse="arguments pour traitement 'Aptitude' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste d'accronyme d'essences séparées par une virgule, ou une liste de carte d'aptitude\n";}

    return aResponse;
}

bool stationDescResource::checkTool(std::string aTool){
    bool aRes(0);

    if (mDico->hasLayerBase(aTool)){ aRes=1;}
    // traitements qui ne sont pas des cartes
    if (aTool=="hdom" | aTool=="compo"  | aTool=="aptitude"){ aRes=1;}

    //if (aRes==0){mResponse="Aucune couche ou traitement de ce nom. Voir la liste sur forestimator.gembloux.ulg.ac.be/api/help";}

    return aRes;
}

OGRGeometry * stationDescResource::checkPolyg(std::string aPolyg){
    //bool aRes(0);
    OGRGeometry * pol=NULL;
    // lecture du polygone
    OGRSpatialReference src;
    /*src.importFromWkt()
     OGRErr err=src.SetWellKnownGeogCS( "EPSG:31370" );
     std::cout << "OGR error setwellKnownGeog : " << err << std::endl; // failure! je sais pas pk
    */
    OGRErr err=OGRGeometryFactory::createFromWkt(aPolyg.c_str(),NULL,&pol);
    std::cout << "createFromWkt OGR error : " << err << std::endl;
    //std::cout << "src " << src.exportToProj4();
    if (err==OGRERR_NONE && pol!=NULL && pol->IsValid()){} else {OGRGeometryFactory::destroyGeometry(pol);}//aRes=1;}
    if (pol!=NULL) {
        int aSurfha=OGR_G_Area(pol)/10000;
        if (aSurfha>globMaxSurf){OGRGeometryFactory::destroyGeometry(pol);}
    }
    //if (aRes==0){mResponse="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370).";}//Wt::WText::tr("api.msg.error.polyg1").toUTF8();}}
    return pol;
}

std::vector<std::string> stationDescResource::parseAptArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"HE_FEE","CS_FEE","CP_FEE","EP_FEE","DO_FEE","ME_FEE"};} else{
        std::vector<std::string> aVAptCarte;
        boost::split( aVAptCarte,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aVAptCarte){
            if (mDico->accroEss2Nom(code)!=""){ code=code+"_FEE";}
            if (mDico->hasLayerBase(code) & (mDico->getLayerBase(code)->getCatLayer()==TypeLayer::FEE | mDico->getLayerBase(code)->getCatLayer()==TypeLayer::CS)){
                aRes.push_back(code);
            }
        }
    }
    //if (aRes.size()==0){mResponse="arguments pour traitement 'Aptitude' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste d'accronyme d'essences séparées par une virgule, ou une liste de carte d'aptitude\n"; }
    return aRes;
}

std::vector<std::string> stationDescResource::parseHdomArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"MNH2019"};} else{
        std::vector<std::string> aV;
        boost::split( aV,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aV){
            if (mDico->hasLayerBase(code) & (mDico->getLayerBase(code)->TypeCart()==TypeCarte::MNH)){
                aRes.push_back(code);
            }
        }
    }
    return aRes;
}
std::vector<std::string> stationDescResource::parseCompoArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"COMPO1","COMPO2","COMPO3","COMPO4","COMPO5","COMPO6","COMPO7","COMPO8","COMPO9"};} else{
        std::vector<std::string> aV;
        boost::split( aV,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aV){
            if (mDico->hasLayerBase(code) & (mDico->getLayerBase(code)->TypeCart()==TypeCarte::Composition)){
                aRes.push_back(code);
            }
        }
    }
    return aRes;
}

std::map<int,double> stationDescResource::simplifieAptStat(std::map<int,double> aStat){
    std::map<int,double> aRes;
    for (auto kv : aStat){
        int apt=kv.first;
        double propSurf=kv.second;
        int aptContr=mDico->AptContraignante(apt);
        if (aRes.find(aptContr)!=aRes.end()){ aRes.at(aptContr)+=propSurf;}else{aRes.emplace(std::make_pair(aptContr,propSurf));}
    }
    // avec clé 1, 2, 3, 4, 11
    return aRes;
}
