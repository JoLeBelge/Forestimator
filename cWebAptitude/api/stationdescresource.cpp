#include "cdicoapt.h"

int globMaxSurf(200);
std::string nameDendroTool("dendro2018");
extern bool globTest;

std::string cDicoApt::geoservice(std::string aTool, std::string aArgs, std::string aPolyg, bool xml){

    //if (globTest) {std::cout << "Forestimator API " << aTool << " aArgs " << aArgs << " polygon " << aPolyg << std::endl;}
    // je suis pas dans une appli Wt donc je n'ai pas accès au docroot malheureusement...
    //Wt::WMessageResourceBundle msg();
    //msg.use(docRoot() + "/forestimator");
    std::string aResponse;

    GDALAllRegister();
    if (checkTool(aTool)){

        // outils différent, car je teste janvier 2022 1) la réponse en xml et 2) l'api sur tout un shp
        //if(aTool=="dendro2018"){

            // finalement je vais me rabattre sur l'interface graphique de forestimator, car c'est le seul moyen que j'ai de facilement uploader un shp. Je pourrais le faire en envoyer par ex un KML dans une requete, mais c'est assez difficile de rester propre. et l'upload de fichier sans passer par fileupload, je n'y parviens pas.

        //} else {

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
                            //aResponse+="pas disponible pour le moment\n";
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

                } else {
                    if (globTest) {std::cout << " API sur layerBase " << std::endl;}

                    std::shared_ptr<layerBase> l=getLayerBase(aTool);
                    // analyse surfacique ; basic stat pour les var continue
                    switch (l->getTypeVar()) {
                    case TypeVar::Continu:{
                        std::cout << " api compute basic stat on polyg " << std::endl;
                        basicStat stat=l->computeBasicStatOnPolyg(pol);
                        std::cout << " done " << std::endl;
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

    } else {aResponse="arguments pour geotraitement ; vous avez rentré une valeur mais qui semble fausse. Consultez la page d'aide.\n";}

    return aResponse;
}

bool cDicoApt::checkTool(std::string aTool){
    bool aRes(0);

    if (hasLayerBase(aTool)){ aRes=1;}
    // traitements qui ne sont pas des cartes
    if (aTool=="hdom"   | aTool=="aptitude" | aTool=="dendro2018" ){ aRes=1;} //| aTool=="compo"

    //if (aRes==0){mResponse="Aucune couche ou traitement de ce nom. Voir la liste sur forestimator.gembloux.ulg.ac.be/api/help";}

    return aRes;
}

OGRGeometry * cDicoApt::checkPolyg(std::string aPolyg){
    //bool aRes(0);
    OGRGeometry * pol=NULL;
    // lecture du polygone
    /*OGRSpatialReference src;
    src.importFromWkt()
     OGRErr err=src.SetWellKnownGeogCS( "EPSG:31370" );
     std::cout << "OGR error setwellKnownGeog : " << err << std::endl; // failure! je sais pas pk
    */
    OGRErr err=OGRGeometryFactory::createFromWkt(aPolyg.c_str(),NULL,&pol);
    if(globTest){std::cout << "createFromWkt OGR error : " << err << std::endl;}
    //std::cout << "src " << src.exportToProj4();
    // isValid() fonctionne mais par contre le destroyGeom doit être suivi d'un pol=NULL sinon bug
    if (err==OGRERR_NONE && pol!=NULL){
        pol->MakeValid();
        // j'ai des pol invalides qui sont des multipolygones avec self intersection, je garde que le premier polygone. solution rapide...
        if (!pol->IsValid() & pol->toGeometryCollection()->getNumGeometries()>1){
          if(OGR_G_Area(pol->toGeometryCollection()->getGeometryRef(0))>100 & pol->toGeometryCollection()->getGeometryRef(0)->IsValid()){
             pol=pol->toGeometryCollection()->getGeometryRef(0);
          }
        }
        if(pol->IsValid()){
             //std::cout << " geométrie valide " << pol->getGeometryName() << std::endl;
        int aSurfha=OGR_G_Area(pol)/10000;
        if (aSurfha>globMaxSurf){OGRGeometryFactory::destroyGeometry(pol);pol=NULL;}
        } else {
            std::cout << " geométrie invalide " << pol->getGeometryName() << " nombre de geometrie " << pol->toGeometryCollection()->getNumGeometries()<< std::endl;
            OGRGeometryFactory::destroyGeometry(pol);pol=NULL;}
    } else {
        std::cout << " OGRErr lors de l'import du wkt" << std::endl;
        OGRGeometryFactory::destroyGeometry(pol);pol=NULL;}

    //if (aRes==0){mResponse="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370).";}//Wt::WText::tr("api.msg.error.polyg1").toUTF8();}}
    if(globTest){std::cout << "checkPolyg API done "<< std::endl;}
    return pol;
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
