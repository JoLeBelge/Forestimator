#include "cdicoapt.h"

std::string dirBD("/home/jo/Documents/carteApt/Forestimator/carteApt/data/aptitudeEssDB.db");
bool globTest(0);
int globMaxSurf(200);


cDicoApt::cDicoApt(std::string aBDFile):cdicoAptBase(aBDFile)
{

    if (openConnection()){
        std::cout << " bd pas ouverte!!!\n"<< std::endl;
    } else {

        if (globTest){std::cout << "cnsw" << std::endl;}
        mPedo= std::make_shared<cnsw>(*db_);

        if (globTest){std::cout << "cadastre" << std::endl;}
        mCadastre= std::make_shared<cadastre>(*db_);
        if (globTest){   std::cout << "cadastre done" << std::endl;}

        sqlite3_stmt * stmt;

        

        std::string SQLstring="SELECT code,label,expert FROM groupe_couche ORDER BY id;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                bool expert(1);
                if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){expert=sqlite3_column_int( stmt, 2 );}
                Dico_groupeLabel.emplace(std::make_pair(aA,aB));
                Dico_groupeExpert.emplace(std::make_pair(aA,expert));
                Dico_groupe.push_back(aA);// juste pour avoir les groupes dans l'ordre
            }
        }
        sqlite3_finalize(stmt);



        SQLstring="SELECT raster_val,Nom FROM dico_topo;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                int aA=sqlite3_column_int( stmt, 0 );
                Dico_topo.emplace(std::make_pair(aA,aB));
            }
        }
        sqlite3_finalize(stmt);
        // j'aimerai charger toutes les couleurs que j'ai dans le dictionnaire, y compris les couleurs qui sont dans le dictionnaire des couches ex composition, affin que je puisse créer un style pour chaque couleur et l'appliquer au Model dans layerstatchart
        SQLstring="SELECT DISTINCT nom_dico FROM fichiersGIS WHERE nom_dico IS NOT NULL;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
                std::string nom_dico=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                SQLstring="SELECT col FROM "+ nom_dico +";";

                sqlite3_stmt * stmt2;
                sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt2, NULL );
                while(sqlite3_step(stmt2) == SQLITE_ROW)
                {
                    if (sqlite3_column_type(stmt2, 0)!=SQLITE_NULL){
                        std::string col=std::string( (char *)sqlite3_column_text( stmt2, 0 ) );
                        // soit c'est un identifiant de couleur que j'ai déjà dans mon dictionnaire, soit c'est un code hexadécimal
                        if (colors.find(col)==colors.end()){
                            // problème quand c'est le code hexadécimal ,c'est que la fonction getCol fonctionne avec un nom de couleur.. donc le nom doit-être le mm que le code hexa, comme ça c'est pa ambigu. sauf que c'est peut-être pas opportun d'avoir un diaise en début de nom..

                            //std::cout << "creation couleur nom " << col.substr(1,col.size()) << std::endl;
                            colors.emplace(std::make_pair(col,std::make_shared<color>(col,col.substr(1,col.size()))));
                        }
                    }
                }
                sqlite3_finalize(stmt2);
            }
        }
        sqlite3_finalize(stmt);


        // toutes les layerbase
        if (globTest){   std::cout << "crée toute les layerbase " << std::endl;}
        for (auto & pair : Dico_RasterType){
            std::shared_ptr<layerBase> l=std::make_shared<layerBase>(pair.first,this);
            if (l->getCatLayer()!=TypeLayer::Externe & !l->rasterExist()){
                std::cout << "Attention layerBase " << l->Code() << ", fichier " << l->getPathTif() << " inexistant" << std::endl;
            }

            mVlayerBase.emplace(std::make_pair(pair.first,l));
        }
        //std::cout << "close connection (dicoApt)" << std::endl;
        closeConnection();

    }

    std::cout << "done " << std::endl;
    boost::filesystem::create_directories(File("TMPDIR"));
    //std::cout << "Dico code essence --> nom essence francais a "<< Dico_codeEs2NomFR.size() << " elements \n" << std::endl;
    //std::cout << "Dico gis file a "<< Dico_GISfile.size() << " elements \n" << std::endl;
}


std::map<int,std::string> cDicoApt::getDicoRaster(std::string aCode){
    std::map<int,std::string> aRes;

    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT nom_dico, nom_field_raster, nom_field_value, condition FROM "+RasterTable(aCode)+" WHERE Code='"+ aCode+"';";
    //std::cout << SQLstring << std::endl;
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    // une seule ligne
    std::string nom_dico,field_raster,field_value,cond("");
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
            nom_dico=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
            field_raster=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
            field_value=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
            if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL )  cond=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
        }
    }
    SQLstring="SELECT "+field_raster+","+field_value+" FROM "+ nom_dico ;
    if (cond!=""){ SQLstring=SQLstring+" WHERE "+cond+";";} else {SQLstring=SQLstring+";";}
    //std::cout << SQLstring << std::endl;
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
            int aA=sqlite3_column_int( stmt, 0 );
            std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
            aRes.emplace(std::make_pair(aA,aB));
        }
    }
    sqlite3_finalize(stmt);

    return aRes;
}

std::map<int,std::shared_ptr<color>> cDicoApt::getDicoRasterCol(std::string aCode){
    std::map<int,std::shared_ptr<color>> aRes;

    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT nom_dico, nom_field_raster, nom_field_value, condition FROM "+RasterTable(aCode)+" WHERE Code='"+ aCode+"';";
    //std::cout << SQLstring << std::endl;
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    // une seule ligne
    std::string nom_dico,field_raster,field_value,cond("");
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
            nom_dico=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
            field_raster=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
            field_value=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
            if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL )  cond=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
        }
    }
    sqlite3_finalize(stmt);
    SQLstring="SELECT "+field_raster+", col FROM "+ nom_dico ;
    if (cond!=""){ SQLstring=SQLstring+" WHERE "+cond+";";} else {SQLstring=SQLstring+";";}
    //std::cout << SQLstring << "\n\n" << std::endl;
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){ // attention, la colonne col peut être vide!! ha ben non plus maintenant.
            int aA=sqlite3_column_int( stmt, 0 );
            std::string aB("");
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL ) {aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );}
            if (aB.substr(0,1)=="#") {
                //if (globTest){std::cout << " ajout dans dicoCol " << aA << " , col " << aB << " table" << nom_dico << std::endl;}
                // il faut d'office l'ajouter au vecteur colors, car les styles sont créé via ce vecteur
                colors.emplace(std::make_pair(aB,std::make_shared<color>(aB,aB)));
            }

            aRes.emplace(std::make_pair(aA,getColor(aB)));
        }
    }
    sqlite3_finalize(stmt);
    return aRes;
}

bool cDicoApt::hasWMSinfo(std::string aCode){
    return Dico_WMS.find(aCode)!=Dico_WMS.end();
}

WMSinfo * cDicoApt::getWMSinfo(std::string aCode){
    WMSinfo * aRes;
    if (Dico_WMS.find(aCode)!=Dico_WMS.end()){
        aRes=&Dico_WMS.at(aCode);
    };
    return aRes;
}


TypeCarte str2TypeCarte(const std::string& str)
{
    TypeCarte aRes=SS;
    if(str == "NH") aRes=NH;
    else if(str == "NT") aRes=NT;
    else if(str == "Topo") aRes=Topo;
    else if(str == "AE") aRes=AE;
    else if(str == "SS") aRes=SS;
    else if(str == "ZBIO") aRes=ZBIO;
    else if(str == "CSArdenne") aRes=CSArdenne;
    else if(str == "CSLorraine") aRes=CSLorraine;
    else if(str == "Composition") aRes=Composition;
    else if(str == "MNH") aRes=MNH;
    return aRes;
}

TypeVar str2TypeVar(const std::string& str){
    TypeVar aRes=TypeVar::Classe;
    if(str == "Continu") {
        aRes=TypeVar::Continu;
    }
    return aRes;
}

TypeLayer str2TypeLayer(const std::string& str)
{
    TypeLayer aRes=TypeLayer::Station;
    if(str == "Station") aRes=TypeLayer::Station;
    if(str == "Peuplement") aRes=TypeLayer::Peuplement;
    if(str == "Externe") aRes=TypeLayer::Externe;
    if(str == "FEE") aRes=TypeLayer::FEE;
    if(str == "CS") aRes=TypeLayer::CS;
    if(str == "KK") aRes=TypeLayer::KK;
    //if (globTest){std::cout << "str2TypeLayer " << str << " , " << (int (aRes)) << std::endl;}
    return aRes;
}

ST::ST(cDicoApt * aDico):mDico(aDico),mNT(666),mNH(666),mZBIO(666),mTOPO(666),mActiveEss(NULL),HaveEss(0),mSt(0),mEmpty(1)
{

}

void ST::vider()
{
    mNT=666;
    mNH=666;
    mZBIO=666;
    mTOPO=666;
    HaveEss=0;
    mSt=666;
    hasFEEApt=0;
    mEmpty=1;
}

std::string removeAccents(std::string aStr){
    boost::replace_all(aStr, "é", "e");
    boost::replace_all(aStr, "è", "e");
    boost::replace_all(aStr, "ê", "e");
    boost::replace_all(aStr, "ï", "i");
    boost::replace_all(aStr, "î", "i");
    boost::replace_all(aStr, "â", "a");
    return aStr;
}

OGRGeometry * cDicoApt::checkPolyg(std::string aPolyg, int maxSurf){

    OGRGeometry * pol=NULL;
    // lecture du polygone
    OGRErr err=OGRGeometryFactory::createFromWkt(aPolyg.c_str(),NULL,&pol);

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
            if (aSurfha>maxSurf){OGRGeometryFactory::destroyGeometry(pol);pol=NULL;
                std::cout << " surface de géométrie trop important :" << std::to_string(aSurfha) << " ..." << std::endl;
            }
        } else {
            std::cout << " geométrie invalide " << pol->getGeometryName() << " nombre de geometrie " << pol->toGeometryCollection()->getNumGeometries()<< std::endl;
            OGRGeometryFactory::destroyGeometry(pol);pol=NULL;}
    } else {
        std::cout << " OGRErr lors de l'import du wkt" << std::endl;
        if(globTest){std::cout << "createFromWkt OGR error : " << err << std::endl;}
        OGRGeometryFactory::destroyGeometry(pol);pol=NULL;}

    //if (aRes==0){mResponse="Veillez utiliser le format wkt pour le polygone (projeté en BL72, epsg 31370).";}//Wt::WText::tr("api.msg.error.polyg1").toUTF8();}}
    //if(globTest){std::cout << "checkPolyg API done "<< std::endl;}
    return pol;
}

std::string cDicoApt::geoservice(std::string aTool, std::string aArgs, std::string aPolyg, typeAna aType, bool xml){

    //if (globTest) {std::cout << "Forestimator API " << aTool << " aArgs " << aArgs << " polygon " << aPolyg << std::endl;}

    std::string aResponse;

    GDALAllRegister();
    if (checkTool(aTool)){

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
                         //   statHdomBase stat(l,pol,1);
                         //   basicStat bs= stat.bshdom();
                         //   if (!xml){aResponse+=l->Code()+";"+bs.getMean()+";"+bs.getCV()+";"+bs.getMax()+";"+bs.getMin()+";"+bs.getNb()+"\n";}
                         /*  else { aResponse="<code_mnh>"+ l->Code()+ "</code_mnh>\n"
                                        +"<moy>"+ bs.getMean()+ "</moy>\n"
                                        +"<cv>"+ bs.getCV()+ "</cv>\n"
                                        +"<max>"+ bs.getMax()+ "</max>\n"
                                        +"<min>"+ bs.getMin()+ "</min>\n"
                                        +"<nb>"+ bs.getNb()+ "</nb>\n";}
                                        */
                            aResponse="calcul de hdom indisponible pour l'instant\n";
                        }
                    }
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

                        // option pour avoir uniquement la classe majoritaire
                        if (aArgs=="maj"){
                      std::pair<int,double> p= l->valMajoritaire(pol);
                      //aResponse+="maj;pct\n";
                      aResponse+=std::to_string(p.first)+";"+roundDouble(p.second,0);//+"\n";
                        } else {

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

                        }

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
                if (aTool=="hdom"   | aTool=="aptitude" ){
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
    if (aTool=="hdom"   | aTool=="aptitude" | aTool=="CNSW"){ aRes=1;}
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
