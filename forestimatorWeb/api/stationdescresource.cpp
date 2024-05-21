//#include "cdicoapt.h"
#include "stationdescresource.h"

int globMaxSurf(200);
std::string nameDendroTool("dendro2018");
extern bool globTest;

std::string cDicoApt::geoservice(std::string aTool, std::string aArgs, std::string aPolyg, typeAna aType, bool xml){

    //if (globTest) {std::cout << "Forestimator API " << aTool << " aArgs " << aArgs << " polygon " << aPolyg << std::endl;}
    // je suis pas dans une appli Wt donc je n'ai pas accès au docroot
    //Wt::WMessageResourceBundle msg();
    //msg.use(docRoot() + "/forestimator");
    std::string aResponse;

    GDALAllRegister();
    if (checkTool(aTool)){

        // outils différent, car je teste janvier 2022 1) la réponse en xml et 2) l'api sur tout un shp
        //if(aTool=="dendro2018"){

        // finalement je vais me rabattre sur l'interface graphique de forestimator, car c'est le seul moyen que j'ai de facilement uploader un shp. Je pourrais le faire en envoyer par ex un KML dans une requete, mais c'est assez difficile de rester propre. et l'upload de fichier sans passer par fileupload, je n'y parviens pas.

        //} else {
        switch(aType){
        case typeAna::surfacique:{
            OGRGeometry * pol=checkPolyg(aPolyg);
            if (pol!=NULL){
                if (aTool=="hdom"){
                    std::vector<std::string> VMNH=parseHdomArg(aArgs);
                    if (VMNH.size()==0){aResponse="arguments pour traitement 'hdom' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste de code de couche MNH séparées par une virgule\n";
                    }else{
                        if (!xml){aResponse+="code_mnh;moy;cv;max;min;nb\n";}
                        for (std::string code : VMNH){
                            std::shared_ptr<layerBase> l=getLayerBase(code);
                            statHdomBase stat(l,pol,1);
                            basicStat bs= stat.bshdom(); // a modifier quand j'aurai fini d'encoder tout les modèles de Jérome
                            if (!xml){aResponse+=l->Code()+";"+bs.getMean()+";"+bs.getCV()+";"+bs.getMax()+";"+bs.getMin()+";"+bs.getNb()+"\n";}
                            else { aResponse="<code_mnh>"+ l->Code()+ "</code_mnh>\n"
                                        +"<moy>"+ bs.getMean()+ "</moy>\n"
                                        +"<cv>"+ bs.getCV()+ "</cv>\n"
                                        +"<max>"+ bs.getMax()+ "</max>\n"
                                        +"<min>"+ bs.getMin()+ "</min>\n"
                                        +"<nb>"+ bs.getNb()+ "</nb>\n";}
                        }
                    }
                } else if (aTool==nameDendroTool){
                    //if (globTest) {std::cout << "dendro 2018 api " << std::endl;}
                    if (!xml){aResponse+="hdom;vha;gha;nha;cmoy\n";}
                    std::shared_ptr<layerBase> l=getLayerBase("MNH2018P95");
                    statDendroBase stat(l,pol,1);
                    if (!xml){aResponse+=stat.getHdom()+";"+stat.getVha()+";"+stat.getGha()+";"+stat.getNha()+";"+stat.getCmoy()+"\n";
                    }else {
                        aResponse=putInBalise(stat.getHdom(),"hdom")
                                +putInBalise(stat.getVha(),"vha")
                                +putInBalise(stat.getGha(),"gha")
                                +putInBalise(stat.getNha(),"nha")
                                +putInBalise(stat.getCmoy(),"cmoy");
                    }

                    /* }else if (aTool=="compo"){
                    std::vector<std::string> VCOMPO=parseCompoArg(aArgs);
                    if (VCOMPO.size()==0){aResponse="arguments pour traitement 'compo' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste de code de couche de probabilité de présence d'une essence forestière, séparées par une virgule\n";
                    }else{
                        std::vector<std::shared_ptr<layerBase>> Vlay;
                        for (std::string code : VCOMPO){
                            Vlay.push_back(mDico->getLayerBase(code));
                        }
                        statCompo stat(Vlay,mDico,pol);
                        aResponse+=stat.getAPIresult();
                    }*/

                }else if(aTool=="aptitude"){
                    std::vector<std::string> VCApt=parseAptArg(aArgs);
                    aResponse+="code_es;type;O;T;TE;E;I\n";
                    for (std::string code : VCApt){
                        std::shared_ptr<layerBase> l=getLayerBase(code);
                        std::map<int,double> stat=l->computeStat2(pol);
                        aResponse+=l->EssCode()+";"+l->getCatLayerStr();
                        std::map<int,double> statSimp=simplifieAptStat(stat);
                        for (auto kv:statSimp){
                            aResponse+=";"+roundDouble(kv.second);
                        }
                        aResponse+="\n";
                    }
                }else if(aTool=="CNSW"){
                    surfPedo surf(mPedo,pol);
                    aResponse+=surf.getSummaryAPI();

                } else if (hasLayerBase(aTool)) {
                    //if (globTest) {std::cout << " API sur layerBase " << std::endl;}

                    std::shared_ptr<layerBase> l=getLayerBase(aTool);
                    // analyse surfacique ; basic stat pour les var continue
                    switch (l->getTypeVar()) {
                    case TypeVar::Continu:{
                        //std::cout << " api compute basic stat on polyg " << std::endl;
                        basicStat stat=l->computeBasicStatOnPolyg(pol);
                        //std::cout << " done " << std::endl;
                        if (!xml){
                            aResponse+="mean;"+stat.getMean()+"\n";
                            aResponse+="max;"+stat.getMax()+"\n";
                            aResponse+="sd;"+stat.getSd()+"\n";
                        } else {
                            aResponse=putInBalise(stat.getMean(),"mean")
                                    +putInBalise(stat.getMax(),"max")
                                    +putInBalise(stat.getSd(),"sd");
                        }
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
                        bool test(1);// detecte la première ligne
                        for (auto kv:stat){

                            if (!xml){
                                if (test){
                                    aL2=std::to_string(kv.first);
                                    aL1=roundDouble(kv.second);
                                    test=0;
                                }else{
                                    aL2+=";"+std::to_string(kv.first);
                                    aL1+=";"+roundDouble(kv.second);
                                }
                            } else {
                                // ici je met trois balise ; nom du field, valeur raster , et pourcentage
                                aResponse+="<classe>";
                                aResponse+=putInBalise(l->getValLabel(kv.first),"classeName");
                                aResponse+=putInBalise(std::to_string(kv.first),"classeRasterVal");
                                aResponse+=putInBalise(roundDouble(kv.second),"pourcentage");
                                aResponse+="</classe>";
                            }
                        }

                        if (!xml){aResponse+=aL2+"\n"+aL1+"\n";}

                        break;
                    }
                    default:
                        break;
                    }
                }
                OGRGeometryFactory::destroyGeometry(pol);
            } else {
                if (!xml){
                    aResponse="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370). La géométrie du polygone (ou du multipolygone) doit être valide et sa surface de maximum "+std::to_string(globMaxSurf)+"ha";
                } else {
                    aResponse="<error>La géométrie du polygone (ou du multipolygone) doit être valide et sa surface de maximum "+std::to_string(globMaxSurf)+"ha</error>";
                }
            }
            break;
        }
        case typeAna::ponctuel:{
            // créer le point
            if (globTest){std::cout << "api analyse ponctuelle" << std::endl;}
            OGRPoint * pt=checkPoint(aPolyg);
            if (pt!=NULL){
                if (aTool=="hdom"   | aTool=="aptitude" | aTool=="dendro2018" ){
                    aResponse="pas de traitement ponctuel pour cet outil";
                } else if(aTool=="CNSW"){
                    ptPedo ptPed=ptPedo(mPedo,pt->getX(),pt->getY());
                    aResponse=ptPed.displayAllInfoAPI();
                }else {
                    std::shared_ptr<layerBase> l=getLayerBase(aTool);
                    std::string typeOut=parsePointArg(aArgs);
                    int aVal=l->getValue(pt->getX(),pt->getY());
                    if (typeOut=="val"){
                    aResponse=std::to_string(aVal);
                    }
                    if (typeOut=="txt"){
                       aResponse=l->getValLabel(aVal);
                    }
                }
            } else {
                aResponse="géométrie du point invalide ";
            }
            break;
        }
        case typeAna::dicoTable:{
            if (globTest){std::cout << "api table dictionnaire" << std::endl;}
            if (hasLayerBase(aTool)) {
                std::shared_ptr<layerBase> l=getLayerBase(aTool);
                aResponse=l->getDicoValStr();
            }
            break;
        }
        }

    } else {aResponse="arguments pour geotraitement ; vous avez rentré une valeur mais qui semble fausse. peut-être le nom de la couche ou du traitement. Consultez la page d'aide.\n";}

    return aResponse;
}

bool cDicoApt::checkTool(std::string aTool){
    bool aRes(0);
    if (hasLayerBase(aTool)){ aRes=1;}
    // traitements qui ne sont pas des cartes
    if (aTool=="hdom"   | aTool=="aptitude" | aTool=="dendro2018" | aTool=="CNSW"){ aRes=1;} //| aTool=="compo"
    return aRes;
}

OGRPoint * cDicoApt::checkPoint(std::string aPolyg){
    OGRGeometry * pol=NULL;
    OGRPoint * pt=NULL;
    OGRErr err=OGRGeometryFactory::createFromWkt(aPolyg.c_str(),NULL,&pol);
    if(globTest){std::cout << "createFromWkt OGR error : " << err << std::endl;}
    if (err==OGRERR_NONE && pol!=NULL){
        pt=pol->toPoint();
    }
    return pt;
}

std::vector<std::string> cDicoApt::parseAptArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"HE_FEE","CS_FEE","CP_FEE","EP_FEE","DO_FEE","ME_FEE"};} else{
        std::vector<std::string> aVAptCarte;
        boost::split( aVAptCarte,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aVAptCarte){
            if (accroEss2Nom(code)!=""){ code=code+"_FEE";}
            if (hasLayerBase(code) & (getLayerBase(code)->getCatLayer()==TypeLayer::FEE | getLayerBase(code)->getCatLayer()==TypeLayer::CS)){
                aRes.push_back(code);
            }
        }
    }
    //if (aRes.size()==0){mResponse="arguments pour traitement 'Aptitude' ; vous avez rentré une valeur mais qui semble fausse. Entrez une liste d'accronyme d'essences séparées par une virgule, ou une liste de carte d'aptitude\n"; }
    return aRes;
}

std::vector<std::string> cDicoApt::parseHdomArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"MNH2019"};} else{
        std::vector<std::string> aV;
        boost::split( aV,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aV){
            if (hasLayerBase(code) & (getLayerBase(code)->TypeCart()==TypeCarte::MNH)){
                aRes.push_back(code);
            }
        }
    }
    return aRes;
}

std::string cDicoApt::parsePointArg(std::string aArgs){
    std::string aRes;
    if (aArgs==""){ aRes="val";} else if(aArgs=="txt"){aRes="txt";}
    return aRes;
}

std::vector<std::string> cDicoApt::parseCompoArg(std::string aArgs){
    std::vector<std::string> aRes;
    if (aArgs==""){ aRes={"COMPO1","COMPO2","COMPO3","COMPO4","COMPO5","COMPO6","COMPO7","COMPO8","COMPO9"};} else{
        std::vector<std::string> aV;
        boost::split( aV,aArgs,boost::is_any_of(","),boost::token_compress_on);
        for (std::string code: aV){
            if (hasLayerBase(code) & (getLayerBase(code)->TypeCart()==TypeCarte::Composition)){
                aRes.push_back(code);
            }
        }
    }
    return aRes;
}

std::map<int,double> cDicoApt::simplifieAptStat(std::map<int,double> aStat){
    std::map<int,double> aRes;
    for (auto kv : aStat){
        int apt=kv.first;
        double propSurf=kv.second;
        int aptContr=AptContraignante(apt);
        if (aRes.find(aptContr)!=aRes.end()){ aRes.at(aptContr)+=propSurf;}else{aRes.emplace(std::make_pair(aptContr,propSurf));}
    }
    // avec clé 1, 2, 3, 4, 11
    return aRes;
}

std::string putInBalise(std::string aCont,std::string aBalise){

    return "<"+aBalise+">"+aCont+"</"+aBalise+">\n";

}

//virtual void handleRequest(const Wt::Http::Request &request, Wt::Http::Response &response) override
void stationDescResource::handleRequest(const Http::Request &request,Http::Response &response){

    if (request.path().substr(request.path().size()-4,4)=="help"){
        response.addHeader("Content-Type","text/plain; charset=utf-8");
        response.out() << "FORESTIMATOR API short help \n"
                          "---------------------------\n\n"
                          "\nListe des traitements pour analyse surfacique (analyse spécifique sur une couche ou analyse standard sur plusieurs couches) \n"
                          "----------------------------------------------------------------------------------------------------\n"
                          "hdom\n"
                          //"compo\n"
                          "aptitude\n"
                          "CNSW\n"
                          +nameDendroTool+"\n";

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

        //if (params.empty()) response.out() << "(empty)\n";
        std::string aTool,aPolyg(""),aArgs;
        typeAna aMode(typeAna::surfacique);
        for (const auto &param : params) {

            const auto &name = param.first;
            const auto &value = param.second;
            if (name=="tool") {aTool=value;}
            if (name=="toolarg") {aArgs=value;}
            if (name=="pol") {aPolyg=value;}
            if (name=="pt") {aPolyg=value;aMode=typeAna::ponctuel;}
            //response.out() << name << ": " << value << '\n';
        }
        // si pas de polygone mais bien le nom d'une couche, on délivre le dictionnaire de la couche à l'utilisateur
        if (aPolyg==""){aMode=typeAna::dicoTable;
            std::cout << " API : le polygone/ point est de "<< aPolyg << std::endl;
        }

        if (aTool==nameDendroTool) {response.addHeader("Content-Type","text/plain; charset=utf-8");}else {
            response.addHeader("Content-Type","text/plain; charset=utf-8");
        }

        response.out() << mDico->geoservice(aTool,aArgs,aPolyg,aMode);
    }
}

