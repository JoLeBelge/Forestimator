#include "cdicoapt.h"

std::string dirBD("/home/lisein/Documents/carteApt/Forestimator/carteApt/data/aptitudeEssDB.db");
bool globTest(0);

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
        //pour l'instant je ne sélectionne pas les stations qui ne sont pas cartographiées ; celles qui ont été regroupée en une station carto.
        std::string SQLstring="SELECT ZBIO,stat_id,Station_carto FROM dico_station WHERE stat_id=stat_num;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_station[aA].emplace(std::make_pair(aB,aC));
            }
        }
        sqlite3_finalize(stmt);




        SQLstring="SELECT Code, id_projet, version, id_reference, Nom, copyrigth,ordre, NomShort,keep FROM carteMTD;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 8)!=SQLITE_NULL){
                if (sqlite3_column_int( stmt, 8 )){// colonne keep
                    std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                    LayerMTD lMTD;
                    lMTD.setCode(aA);
                    //if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL ){lMTD.setDescr(std::string( (char *)sqlite3_column_text( stmt, 2 ) ));}
                    if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL ){lMTD.setVersion(std::string( (char *)sqlite3_column_text( stmt, 2 ) ));}
                    if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL ){
                        SQLstring="SELECT Description FROM carteMTD_projet WHERE Code='"+std::string( (char *)sqlite3_column_text( stmt, 1 ))+"';";
                        sqlite3_stmt * stmt2;
                        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt2, NULL );
                        while(sqlite3_step(stmt2) == SQLITE_ROW)
                        {
                            if (sqlite3_column_type(stmt2, 0)!=SQLITE_NULL){
                                lMTD.setProjet(std::string( (char *)sqlite3_column_text( stmt2, 0 )) );
                            }
                        }
                        sqlite3_finalize(stmt2);
                    }
                    if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL ){
                        // le champ id est une liste de numéro sépraré par une virgule, il faut parser
                        std::vector<std::string> aVidRef;
                        std::string aListRef=std::string( (char *)sqlite3_column_text( stmt, 3 ));
                        boost::split( aVidRef,aListRef,boost::is_any_of(","),boost::token_compress_on);
                        for (std::string idRef: aVidRef){
                            SQLstring="SELECT ref FROM carteMTD_reference WHERE id="+idRef+";";
                            sqlite3_stmt * stmt2;
                            sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt2, NULL );
                            while(sqlite3_step(stmt2) == SQLITE_ROW)
                            {
                                if (sqlite3_column_type(stmt2, 0)!=SQLITE_NULL){
                                    lMTD.addRef(std::string( (char *)sqlite3_column_text( stmt2, 0 )) );
                                }
                            }
                            sqlite3_finalize(stmt2);
                        }
                    }
                    if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL ){lMTD.setNom(std::string( (char *)sqlite3_column_text( stmt, 4 ) ));}
                    if (sqlite3_column_type(stmt, 5)!=SQLITE_NULL ){lMTD.setCopyR(std::string( (char *)sqlite3_column_text( stmt, 5 ) ));}
                    //std::cout << " done layerMTD " << lMTD.Nom() << std::endl;
                    std::string aB=std::string( (char *)sqlite3_column_text( stmt, 6 ) );
                    if (sqlite3_column_type(stmt, 7)!=SQLITE_NULL ){lMTD.setLabel(std::string( (char *)sqlite3_column_text( stmt, 7 ) ));}
                    Dico_layerMTD.emplace(std::make_pair(aB,lMTD));
                }
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT code,label,expert FROM groupe_couche ORDER BY id;";
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

        SQLstring="SELECT Code,Nom,NomCol FROM dico_caracteristiqueCS;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ));
                Dico_codeKK2Nom.emplace(std::make_pair(aA,aB));
                Dico_codeKK2NomCol.emplace(std::make_pair(aA,aC));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT id,cat_id,nom FROM dico_echelleFact;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );

        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ));
                Dico_echelleFact.emplace(std::make_pair(aA,aB));
                Dico_echelleFactNom.emplace(std::make_pair(aB,aC));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT id,Nom_fr_wal, WalEunis FROM dico_habitat;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ));
                Dico_id2Habitat.emplace(std::make_pair(aA,aB));
                Dico_codeSt2Habitat.emplace(std::make_pair(aC,aB));
                Dico_codeSt2idHab.emplace(std::make_pair(aC,aA));
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT DISTINCT N_cat_pot,Cat_pot FROM dico_echellePotentiel;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));

                int aA=sqlite3_column_int( stmt, 0 );
                Dico_echellePotCat.emplace(std::make_pair(aA,aB));
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
                            colors.emplace(std::make_pair(col,color(col,col.substr(1,col.size()))));
                        }
                    }
                }
                sqlite3_finalize(stmt2);
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT Code_Aptitude,Num,Equiv2Code,OrdreContrainte,Aptitude,col,surcote,souscote,EquCodeNonContr FROM dico_apt;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                int aB=sqlite3_column_int( stmt, 1 );
                std::string aF=std::string( (char *)sqlite3_column_text( stmt, 5 ) );
                Dico_codeApt2col.emplace(std::make_pair(aB,getColor(aF)));
            }
        }
        sqlite3_finalize(stmt);

        if (globTest){   std::cout << "crée toute les essences " << std::endl;}


        // toutes les layerbase
        if (globTest){   std::cout << "crée toute les layerbase " << std::endl;}
        for (auto & pair : Dico_RasterType){
            std::shared_ptr<layerBase> l=std::make_shared<layerBase>(pair.first,this);
            if (l->getCatLayer()!=TypeLayer::Externe & !l->rasterExist()){
                std::cout << " \n Attention layerBase " << l->Code() << ", fichier " << l->getPathTif() << " inexistant" << std::endl;
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

std::map<int,color> cDicoApt::getDicoRasterCol(std::string aCode){
    std::map<int,color> aRes;

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
                if (globTest){std::cout << " ajout dans dicoCol " << aA << " , col " << aB << " table" << nom_dico << std::endl;}
                // il faut d'office l'ajouter au vecteur colors, car les styles sont créé via ce vecteur
                colors.emplace(std::make_pair(aB,color(aB,aB)));
            }

            aRes.emplace(std::make_pair(aA,getColor(aB)));
        }
    }
    sqlite3_finalize(stmt);
    return aRes;
}


std::map<int,color> cDicoApt::getDicoRasterCol(cKKCS * aKK){
    std::map<int,color> aRes;

    sqlite3_stmt * stmt;
    std::string SQLstring("");
    if (aKK->IsFact()){ SQLstring="SELECT DISTINCT cat_id,col FROM dico_echelleFact;";}
    if (aKK->IsPot()){ SQLstring="SELECT DISTINCT N_cat_pot,col FROM dico_echellePotentiel;";}
    if (aKK->IsHabitat()){ SQLstring="SELECT id,col FROM dico_habitat;";}

    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL ){ // attention, la colonne col peut être vide!!
            int aA=sqlite3_column_int( stmt, 0 );
            std::string aB("");
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL ) {aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );}
            aRes.emplace(std::make_pair(aA,getColor(aB)));
        }
    }
    sqlite3_finalize(stmt);

    return aRes;
}



std::map<int,std::map<int,int>> cDicoApt::getKKCS(std::string aColName){
    std::map<int,std::map<int,int>> aRes;

    sqlite3_stmt * stmt;
    // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
    for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
        std::string SQLstring="SELECT stat_num,"+aColName+" FROM caracteristiqueCS WHERE ZBIO="+ std::to_string(zbio)+";";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int station=sqlite3_column_int( stmt, 0 );
                int echelle=sqlite3_column_int( stmt, 1 );
                aRes[zbio].emplace(std::make_pair(station,echelle));
            }
        }
        sqlite3_finalize(stmt);
    }


    return aRes;
}

std::map<int,std::map<int,std::vector<std::string>>> cDicoApt::getHabitatCS(std::string aColName){
    std::map<int,std::map<int,std::vector<std::string>>> aRes;

    sqlite3_stmt * stmt;
    // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
    for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
        std::string SQLstring="SELECT stat_num,"+aColName+" FROM caracteristiqueCS WHERE ZBIO="+ std::to_string(zbio)+";";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int station=sqlite3_column_int( stmt, 0 );
                std::string habitat=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                //std::cout << " station " << station << " habitat : " << habitat << std::endl;
                // découpage de la chaine de char avec séparateur ;
                std::string delim(";");
                std::vector<int> aVPos;
                std::vector<std::string> aVHab;
                size_t last = 0;
                size_t next = 0;
                aVPos.push_back(0);
                while ((next = habitat.find(delim, last)) != std::string::npos)
                {   aVPos.push_back(next);
                    // une manière de baquer le délimitateur du nom
                    aVPos.push_back(next+1);
                    last = next + 1;
                }
                aVPos.push_back(habitat.size());
                //std::cout << " position done , nb elem = " <<aVPos.size() << std::endl;
                // on découpe le nom de l'image
                if (aVPos.size()>0){
                    for (int i(0); i<(aVPos.size()-1) ;i++){
                        int p(aVPos.at(i));
                        int j=aVPos.at(i+1)-p;
                        if (j >1){
                            std::string sub = habitat.substr(p,j);
                            //std::cout << " sub habitat = " << sub << std::endl;
                            aVHab.push_back(sub);
                        }
                    }
                } else aVHab.push_back(habitat);

                aRes[zbio].emplace(std::make_pair(station,aVHab));
            }
        }
        sqlite3_finalize(stmt);
    }


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

std::map<std::string,LayerMTD> * cDicoApt::layerMTD(){return &Dico_layerMTD;}


cKKCS::cKKCS(std::string aCode,cDicoApt * aDico):mCode(aCode),mNom(aDico->codeKK2Nom(aCode)),mDico(aDico)
  ,mNomCol(aDico->codeKK2NomCol()->at(aCode)),mType(Potentiel)
  ,mHabitat(false)
{
    //std::cout << "constructeur KKCS , mCode " << mCode << std::endl;
    if (mCode=="Habitat"){mHabitat=true;mType=Habitats;}

    if(!mHabitat){
        mEchelleCS=aDico->getKKCS(mNomCol);
        //std::cout << this->summary() << std::endl;
    } else {
        mHabitats=aDico->getHabitatCS(mNomCol);
    }
    mDicoCol=mDico->getDicoRasterCol(this);


    if (IsHabitat()){
        mDicoVal=*mDico->id2Hab();
    } else if (IsFact()){
        mDicoVal=*mDico->echelleFactNom() ;
    } else if (IsPot()){
        mDicoVal=*mDico->echellePotCat() ;
    }
    //std::cout << "done " << std::endl;
}

std::string cKKCS::NomCarte(){return mDico->File("OUTDIR")+"KK_CS_"+mCode+".tif";}
std::string cKKCS::shortNomCarte(){return "KK_CS_"+mCode+".tif";}
//std::string cKKCS::NomDirTuile(){return mDico->File("OUTDIR2")+"KK_CS_"+mCode;}

std::string cKKCS::NomMapServerLayer(){return "KK_CS_"+mCode;}
std::string cKKCS::NomMapServerLayerFull(){return "Description stationnelle - "+mNom;}

int cKKCS::getEchelle(int aZbio,int aSTId){
    int aRes(0);
    if (mEchelleCS.find(aZbio)!=mEchelleCS.end()){
        std::map<int,int> * Apt=&mEchelleCS.at(aZbio);
        if (Apt->find(aSTId)!=Apt->end()){
            int aEchelleFine=Apt->at(aSTId);
            // echelle plus grossière, 3 catégories
            aRes=mDico->echelleFact(aEchelleFine);
        }
    }
    return aRes;
}

int cKKCS::getHab(int aZbio,int aSTId){
    int aRes(0);
    if (mHabitats.find(aZbio)!=mHabitats.end()){
        std::map<int,std::vector<std::string>> * hab=&mHabitats.at(aZbio);
        if (hab->find(aSTId)!=hab->end()){
            std::vector<std::string> aVHab=hab->at(aSTId);
            // renvoi l'id du premier habitat
            aRes=mDico->habitatId(aVHab.at(0));
        }
    }
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
