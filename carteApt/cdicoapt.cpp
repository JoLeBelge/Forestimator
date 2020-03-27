#include "cdicoapt.h"

std::string dirBD("/home/lisein/Documents/carteApt/Forestimator/carteApt/data/aptitudeEssDB.db");

cDicoApt::cDicoApt(std::string aBDFile):mBDpath(aBDFile)
{
    std::cout << "chargement des dictionnaires de la BD ..." ;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
          std::cout << " mBDpath " << mBDpath << std::endl;
    } else {
        // dico Ess Nom -- code
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT Ess_FR,Code_FR FROM dico_essences ORDER BY Ess_FR DESC;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aNomEs=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aCodeEs=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                Dico_code2NomFR.emplace(std::make_pair(aCodeEs,aNomEs));
            }
        }

        SQLstring="SELECT Nom,Zbio FROM dico_zbio;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                int aB=sqlite3_column_int( stmt, 1 );
                Dico_ZBIO.emplace(std::make_pair(aB,aA));
            }
        }

        SQLstring="SELECT raster_val,NH FROM dico_raster_nh;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_NH.emplace(std::make_pair(aB,aA));
            }
        }

        SQLstring="SELECT raster_val,NT FROM dico_raster_nt;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_NT.emplace(std::make_pair(aB,aA));
            }
        }

        SQLstring="SELECT ID,concat2 FROM dico_NTNH;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_code2NTNH.emplace(std::make_pair(aB,aA));
                Dico_NTNH2Code.emplace(std::make_pair(aA,aB));
            }
        }
        //pour l'instant je ne sélectionne pas les stations qui ne sont pas cartographiées ; celles qui ont été regroupée en une station carto.
        SQLstring="SELECT ZBIO,stat_id,Station_carto FROM dico_station WHERE stat_id=stat_num;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_station[aA].emplace(std::make_pair(aB,aC));
            }
        }

        // changer la requete en fonction de la machine sur laquelle est installé l'appli
        char userName[20];
        getlogin_r(userName,sizeof(userName));
        std::string s(userName);
        if (s=="lisein"){
        SQLstring="SELECT Code,Dir2,Nom,Type,NomComplet FROM fichiersGIS;";
        } else {
        SQLstring="SELECT Code,Dir,Nom,Type,NomComplet FROM fichiersGIS;";
        }
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_GISfile.emplace(std::make_pair(aA,aB+"/"+aC));

                if ( sqlite3_column_type(stmt, 3)!=SQLITE_NULL && sqlite3_column_type(stmt, 4)!=SQLITE_NULL){
                    std::string aD=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
                    std::string aE=std::string( (char *)sqlite3_column_text( stmt, 4 ) );
                    Dico_RasterType.emplace(std::make_pair(aA,aD));
                    Dico_RasterNomComplet.emplace(std::make_pair(aA,aE));
                }
            }
        }

        SQLstring="SELECT Col,R,G,B FROM dico_color;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL&& sqlite3_column_type(stmt, 3)!=SQLITE_NULL){

                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                int R=sqlite3_column_int( stmt, 1 );
                int G=sqlite3_column_int( stmt, 2 );
                int B=sqlite3_column_int( stmt, 3 );
                colors.emplace(std::make_pair(aA,color(R,G,B)));
            }
        }

        SQLstring="SELECT Code_Aptitude,Num,Equiv2Code,OrdreContrainte,Aptitude,col FROM dico_apt;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                int aB=sqlite3_column_int( stmt, 1 );
                int aC=sqlite3_column_int( stmt, 2 );
                int aD=sqlite3_column_int( stmt, 3 );
                std::string aE=std::string( (char *)sqlite3_column_text( stmt, 4 ) );
                std::string aF=std::string( (char *)sqlite3_column_text( stmt, 5 ) );
                Dico_Apt.emplace(std::make_pair(aA,aB));
                Dico_code2Apt.emplace(std::make_pair(aB,aA));
                Dico_AptDouble2AptContr.emplace(std::make_pair(aB,aC));
                Dico_Apt2OrdreContr.emplace(std::make_pair(aB,aD));
                Dico_code2AptFull.emplace(std::make_pair(aB,aE));
                Dico_codeApt2col.emplace(std::make_pair(aB,getColor(aF)));
            }
        }
        SQLstring="SELECT Code,Nom,NomCol FROM dico_caracteristiqueCS;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
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
        SQLstring="SELECT id,cat_id,nom FROM dico_echelleFact;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
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

        SQLstring="SELECT id,Nom_fr_wal, WalEunis FROM dico_habitat;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
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
        SQLstring="SELECT code,risque,categorie FROM dico_risque;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                int aC=sqlite3_column_int( stmt, 2 );
                Dico_risque.emplace(std::make_pair(aA,aB));
                Dico_risque2Code.emplace(std::make_pair(aB,aA));
                Dico_risqueCategorie.emplace(std::make_pair(aA,aC));
            }
        }

        SQLstring="SELECT DISTINCT N_cat_pot,Cat_pot FROM dico_echellePotentiel;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));

                int aA=sqlite3_column_int( stmt, 0 );
                Dico_echellePotCat.emplace(std::make_pair(aA,aB));
            }
        }

        SQLstring="SELECT raster_val,Nom FROM dico_topo;";
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                int aA=sqlite3_column_int( stmt, 0 );
                Dico_topo.emplace(std::make_pair(aA,aB));
            }
        }


    }

    std::cout << "\nDico code essence --> nom essence francais a "<< Dico_code2NomFR.size() << " elements" << std::endl;
    /*std::cout << "Dico code NH --> nom NH a "<< Dico_NH.size() << " elements" << std::endl;
    std::cout << "Dico code NT --> nom NT a "<< Dico_NT.size() << " elements" << std::endl;
    std::cout << "Dico code NTNH --> nom NH a "<< Dico_code2NTNH.size() << " elements" << std::endl;
    std::cout << "Dico code Zbio --> nom Zone bioclim a "<< Dico_ZBIO.size() << " elements" << std::endl;
    std::cout << "Dico code station --> nom station a "<< Dico_station.size() << " elements" << std::endl;
    std::cout << "Dico code couche SIG --> chemin d'accès a "<< Dico_GISfile.size() << " elements" << std::endl;
    std::cout << "Dico aptitude --> code aptitude a "<< Dico_Apt.size() << " elements" << std::endl;*/
    std::cout << "done" << std::endl ;

    /*for (auto & p : colors){
        std::cout << " couleur " << p.first << " : " << p.second.cat() << std::endl;
    }*/
}

std::map<int,std::map<std::string,int>> cDicoApt::getFEEApt(std::string aCodeEs){
    std::map<int,std::map<std::string,int>> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT CodeNTNH,'1','2','3','4','5','6','7','8','9','10' FROM AptFEE WHERE CODE_ESSENCE='"+ aCodeEs+"';";
        boost::replace_all(SQLstring, "'", "\"");
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int code=sqlite3_column_int( stmt, 0 );
                for (int i(1);i<11;i++){
                    std::string apt=std::string( (char *)sqlite3_column_text( stmt, i ) );
                    // i == zone bioclim
                    std::string codeNTNH=code2NTNH(code);
                    int codeApt=Apt(apt);
                    aRes[i].emplace(std::make_pair(codeNTNH,codeApt));
                }
            }
        }

    }
    return aRes;
}

std::map<int,int> cDicoApt::getZBIOApt(std::string aCodeEs){
    std::map<int,int> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT '1','2','3','4','5','6','7','8','9','10' FROM AptFEE_ZBIO WHERE CODE_ESSENCE='"+ aCodeEs+"';";
        boost::replace_all(SQLstring, "'", "\"");
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
                for (int i(0);i<10;i++){
                    std::string apt=std::string( (char *)sqlite3_column_text( stmt, i ) );
                    // i == zone bioclim
                    int codeApt=Apt(apt);
                    aRes.emplace(std::make_pair(i+1,codeApt));
                }
            }
        }
    }
    return aRes;
}


std::map<int,std::string> cDicoApt::getDicoRaster(std::string aCode){
    std::map<int,std::string> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT nom_dico, nom_field_raster, nom_field_value, condition FROM fichiersGIS WHERE Code='"+ aCode+"';";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                aRes.emplace(std::make_pair(aA,aB));
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
    }

    return aRes;
}

std::map<int,color> cDicoApt::getDicoRasterCol(std::string aCode){
    std::map<int,color> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT nom_dico, nom_field_raster, nom_field_value, condition FROM fichiersGIS WHERE Code='"+ aCode+"';";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
        SQLstring="SELECT "+field_raster+", col FROM "+ nom_dico ;
        if (cond!=""){ SQLstring=SQLstring+" WHERE "+cond+";";} else {SQLstring=SQLstring+";";}
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
        sqlite3_close(db_);
    }
    return aRes;
}


std::map<int,color> cDicoApt::getDicoRasterCol(cKKCS * aKK){
    std::map<int,color> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        std::string SQLstring("");
        if (aKK->IsFact()){ SQLstring="SELECT DISTINCT cat_id,col FROM dico_echelleFact;";}
        if (aKK->IsPot()){ SQLstring="SELECT DISTINCT N_cat_pot,col FROM dico_echellePotentiel;";}
        if (aKK->IsHabitat()){ SQLstring="SELECT id,col FROM dico_habitat;";}

        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
        sqlite3_close(db_);
    }
    return aRes;
}


std::map<int,std::map<int,int>> cDicoApt::getCSApt(std::string aCodeEs){
    std::map<int,std::map<int,int>> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
        for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
            std::string SQLstring="SELECT stat_id,"+aCodeEs+" FROM AptCS WHERE ZBIO="+ std::to_string(zbio)+";";
            //std::cout << SQLstring << std::endl;
            sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                    int station=sqlite3_column_int( stmt, 0 );
                    std::string apt=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                    int codeApt=Apt(apt);
                    aRes[zbio].emplace(std::make_pair(station,codeApt));
                }
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
    }
    return aRes;
}

std::map<int,std::map<int,int>> cDicoApt::getKKCS(std::string aColName){
    std::map<int,std::map<int,int>> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
        for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
            std::string SQLstring="SELECT stat_num,"+aColName+" FROM caracteristiqueCS WHERE ZBIO="+ std::to_string(zbio)+";";
            //std::cout << SQLstring << std::endl;
            sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                    int station=sqlite3_column_int( stmt, 0 );
                    int echelle=sqlite3_column_int( stmt, 1 );
                    aRes[zbio].emplace(std::make_pair(station,echelle));
                }
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
    }
    return aRes;
}

std::map<int,std::map<int,std::vector<std::string>>> cDicoApt::getHabitatCS(std::string aColName){
    std::map<int,std::map<int,std::vector<std::string>>> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;
        // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
        for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
            std::string SQLstring="SELECT stat_num,"+aColName+" FROM caracteristiqueCS WHERE ZBIO="+ std::to_string(zbio)+";";
            //std::cout << SQLstring << std::endl;
            sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
                            //std::cout << " toto " << i << " i "<< std::endl;
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
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
    }
    return aRes;
}

std::map<int,std::map<int,int>> cDicoApt::getRisqueTopo(std::string aCodeEs){
    std::map<int,std::map<int,int>> aRes;
    int rc;
    rc = sqlite3_open(mBDpath.c_str(), &db_);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
    } else {
        sqlite3_stmt * stmt;

        // hors Ardenne
        std::string SQLstring="SELECT Secteurfroid,Secteurneutre,Secteurchaud,fond_Vallee FROM Risque_topoFEE WHERE Code_Fr='"+ aCodeEs+"';";
        boost::replace_all(SQLstring, "'", "\"");
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL && sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
                for (int topo(0);topo<4;topo++){
                    std::string risque=std::string( (char *)sqlite3_column_text( stmt, topo) );
                    int codeRisque=Risque(risque);
                    for (int zbio(3);zbio<10;zbio++){
                        aRes[zbio].emplace(std::make_pair(topo+1,codeRisque));
                    }
                }
            }
        }
        // Ardenne
        SQLstring="SELECT SF_Ardenne,Secteurneutre,Secteurchaud,FV_Ardenne FROM Risque_topoFEE WHERE Code_Fr='"+ aCodeEs+"';";
        boost::replace_all(SQLstring, "'", "\"");
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        // il peut y avoir des null dans les données, dans ce cas  le risque hors ardenne est identique au risque en ardenne
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL)
                for (int topo(0);topo<4;topo++){
                    int codeRisque(0);
                    if (sqlite3_column_type(stmt, topo)==SQLITE_NULL) {
                        // identique à risque hors ardenne
                        codeRisque=aRes.at(3).at(topo+1);
                    } else {
                        std::string risque=std::string( (char *)sqlite3_column_text( stmt, topo) );
                        codeRisque=Risque(risque);
                    }
                    aRes[1].emplace(std::make_pair(topo+1,codeRisque));
                    aRes[2].emplace(std::make_pair(topo+1,codeRisque));
                    aRes[10].emplace(std::make_pair(topo+1,codeRisque));
                }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
    }
    return aRes;
}

cEss::cEss(std::string aCodeEs,cDicoApt * aDico):mCode(aCodeEs),mNomFR(aDico->code2Nom()->at(aCodeEs)),mDico(aDico)
  ,mType(Apt){
    //std::cout << "creation de l'essence " << mNomFR << std::endl;
    // aptitudes du FEE
    mEcoVal=aDico->getFEEApt(mCode);

    mAptCS=aDico->getCSApt(mCode);

    mAptZbio=aDico->getZBIOApt(mCode);

    mRisqueTopo=aDico->getRisqueTopo(mCode);
}

//effectue la confrontation Apt Zbio et AptHydroTrophiue
int cEss::getApt(int aCodeNT, int aCodeNH, int aZbio, bool hierachique){
    int aRes(0);
    //int codeNTNH= mDico->NTNH()->at("h"+std::to_string(aCodeNH)+"t"+std::to_string(aCodeNT));
    std::string codeNTNH= "h"+std::to_string(aCodeNH)+"t"+std::to_string(aCodeNT);
    if (mEcoVal.find(aZbio)!=mEcoVal.end()){
        //aRes=20;
        std::map<std::string,int> * Eco=&mEcoVal.at(aZbio);
        if (Eco->find(codeNTNH)!=Eco->end()){
            aRes=Eco->at(codeNTNH);
        }
    }
    // confrontation de l'aptitude de l'écogramme avec celui de la zone bioclim et choix du plus contraignant
    if (hierachique && mAptZbio.find(aZbio)!=mAptZbio.end()){
        int aZbioApt= mAptZbio.at(aZbio);
        if (mDico->AptContraignante(aRes)<mDico->AptContraignante(aZbioApt)){
            aRes=aZbioApt;
        }
    }
    return aRes;
}

int cEss::getApt(int aZbio,int aSTId){
    int aRes(0);
    if (mAptCS.find(aZbio)!=mAptCS.end()){
        std::map<int,int> * Apt=&mAptCS.at(aZbio);
        if (Apt->find(aSTId)!=Apt->end()){
            aRes=Apt->at(aSTId);
        }
    }
    return aRes;
}

int cEss::getApt(int aZbio){
    int aRes(0);
    if (mAptZbio.find(aZbio)!=mAptZbio.end()){
        aRes=mAptZbio.at(aZbio);
    }
    return aRes;
}

int cEss::corrigAptRisqueTopo(int apt,int topo,int zbio){
    int aRes=apt;
    int risque=getRisque(zbio,topo);
    int catRisque=mDico->risqueCat(risque);
    // situation favorable
    if (catRisque==1){
        aRes=mDico->AptSurcote(apt);
        //std::cout << "Surcote l'aptitude " << mDico->code2Apt(apt) << " vers " << mDico->code2Apt(aRes) << std::endl;
    }
    // risque élevé et très élevé
    if (catRisque==3){aRes=mDico->AptSouscote(apt);}
    //std::cout << "Décote l'aptitude " << mDico->code2Apt(apt) << " vers " << mDico->code2Apt(aRes) << std::endl;}
    return aRes;
}

std::string cEss::printRisque(){
    std::string aRes("Risque pour "+mNomFR+ "------\n");
    for (int zbio(1); zbio<11;zbio++){
        for (int topo(1);topo<5;topo++){
            aRes= aRes+ mDico->ZBIO(zbio) +", topo "+ std::to_string(topo) ;
            aRes= aRes+ ", risque : " + mDico->Risque(getRisque(zbio,topo)) +"\n" ;
        }
    }
    return aRes;
}
std::string cEss::NomCarteAptFEE(){return mDico->Files()->at("OUTDIR")+"aptitudeFEE_"+mCode+".tif";}
std::string cEss::NomDirTuileAptFEE(){return mDico->Files()->at("OUTDIR2")+"FEE_"+mCode;}
std::string cEss::shortNomCarteAptFEE(){return "aptitudeFEE_"+mCode+".tif";}
std::string cEss::NomCarteAptCS(){return mDico->Files()->at("OUTDIR")+"aptitudeCS_"+mCode+".tif";}
std::string cEss::NomDirTuileAptCS(){return mDico->Files()->at("OUTDIR2")+"CS_"+mCode;}
std::string cEss::shortNomCarteAptCS(){return "aptitudeCS_"+mCode+".tif";}


bool cEss::hasRisqueComp(int zbio,int topo){
    bool aRes(0);
    int risque=getRisque(zbio,topo);
    int catRisque=mDico->risqueCat(risque);
    if (catRisque==1 | catRisque==3){aRes=1;}
    return aRes;
}

cKKCS::cKKCS(std::string aCode,cDicoApt * aDico):mCode(aCode),mNom(aDico->codeKK2Nom(aCode)),mDico(aDico)
  ,mNomCol(aDico->codeKK2NomCol()->at(aCode)),mType(Potentiel)
  ,mHabitat(false)
    {

    if (mCode=="Habitat"){mHabitat=true;mType=Habitats;}

    if(!mHabitat){
        mEchelleCS=aDico->getKKCS(mNomCol);
        //std::cout << this->summary() << std::endl;
    } else {
        mHabitats=aDico->getHabitatCS(mNomCol);
    }
    mDicoCol=mDico->getDicoRasterCol(this);
}

std::string cKKCS::NomCarte(){return mDico->Files()->at("OUTDIR")+"KK_CS_"+mCode+".tif";}
std::string cKKCS::NomDirTuile(){return mDico->Files()->at("OUTDIR2")+"KK_CS_"+mCode;}


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

cRasterInfo::cRasterInfo(std::string aCode,cDicoApt * aDico):mDico(aDico),mCode(aCode){
    mNom=mDico->RasterNom()->at(mCode);
    mPathRaster=mDico->Files()->at(mCode);
    mType =str2TypeCarte(mDico->RasterType()->at(mCode));
    mDicoVal=mDico->getDicoRaster(mCode);
    mDicoCol=mDico->getDicoRasterCol(mCode);
}

std::string cRasterInfo::NomTuile(){return mCode;}
std::string cRasterInfo::NomDirTuile(){return mDico->Files()->at("OUTDIR2")+NomTuile();}
std::string cRasterInfo::NomFile(){
    boost::filesystem::path p(mPathRaster);
    return p.stem().c_str();}


TypeCarte str2TypeCarte(const std::string& str)
{
    if(str == "NH") return NH;
    else if(str == "NT") return NT;
    else if(str == "Topo") return Topo;
    else if(str == "AE") return AE;
    else if(str == "SS") return SS;
    else if(str == "ZBIO") return ZBIO;
    else if(str == "CSArdenne") return CSArdenne;
    else if(str == "CSLorraine") return CSLorraine;
}


std::string loadBDpath()
{
    std::string aRes;
    // changer chemin d'accès en fonction de la machine sur laquelle tourne l'app. pour pas devoir modifier en local et se priver des avantages de git
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName);
    if (s=="lisein"){
    aRes="/home/lisein/Documents/carteApt/Forestimator/carteApt/data/aptitudeEssDB.db";
    } else {
    aRes="/data1/Forestimator/carteApt/data/aptitudeEssDB.db";
    }
    //std::ifstream File("./data/BDpath.txt");
    //File >> aRes;
    return aRes;
}
