#include "cdicoapt.h"

std::string dirBD("/home/lisein/Documents/carteApt/Forestimator/carteApt/data/aptitudeEssDB.db");
bool globTest(0);

// memory leak with sqlite3 : https://stackoverflow.com/questions/11126070/sqlite3-prepare-v2-depends-on-scope
// mieux ; https://stackoverflow.com/questions/33432551/possible-memory-leak-in-the-shared-library-of-sqlite3/33435655
// apparemment le pbl viens de sqlite3_open_v2( qui doit prendre comme argument sqlite3 **ppDb, (double pointeur, pas simple!!)

cDicoApt::cDicoApt(std::string aBDFile):mBDpath(aBDFile),ptDb_(NULL)
{
    std::cout << "toto" << std::endl;
    db_=&ptDb_;
    //*db_=NULL;
    std::cout << "constructeur cDicoApt, chemin accès "<< mBDpath << std::endl;
    if (openConnection()){
    std::cout << " bd pas ouverte!!!\n"<< std::endl;
    } else {
        std::cout << "cnsw" << std::endl;
        mPedo= std::make_shared<cnsw>(*db_);
        std::cout << "cadastre" << std::endl;
        mCadastre= std::make_shared<cadastre>(*db_);
        // dico Ess Nom -- code
        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT Ess_FR,Code_FR,prefix FROM dico_essences ORDER BY Ess_FR DESC;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aNomEs=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aCodeEs=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aPrefix=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_codeEs2NomFR.emplace(std::make_pair(aCodeEs,aNomEs));
                Dico_code2prefix.emplace(std::make_pair(aCodeEs,aPrefix));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT Nom,Zbio FROM dico_zbio;";

        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                int aB=sqlite3_column_int( stmt, 1 );
                Dico_ZBIO.emplace(std::make_pair(aB,aA));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT raster_val,NH,posEco, id_groupNH FROM dico_raster_nh;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );

                Dico_NH.emplace(std::make_pair(aB,aA));
                if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){ Dico_NHposEco.emplace(std::make_pair(aB,sqlite3_column_int( stmt, 2 )));}
                if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL){Dico_rasterNH2groupe.emplace(std::make_pair(aB,sqlite3_column_int( stmt, 3 )));}
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT raster_val,NT FROM dico_raster_nt;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_NT.emplace(std::make_pair(aB,aA));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT ID,concat2 FROM dico_NTNH;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_code2NTNH.emplace(std::make_pair(aB,aA));
                Dico_NTNH2Code.emplace(std::make_pair(aA,aB));
            }
        }
        sqlite3_finalize(stmt);
        //pour l'instant je ne sélectionne pas les stations qui ne sont pas cartographiées ; celles qui ont été regroupée en une station carto.
        SQLstring="SELECT ZBIO,stat_id,Station_carto FROM dico_station WHERE stat_id=stat_num;";
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
        // changer la requete en fonction de la machine sur laquelle est installé l'appli
        char userName[20];
        getlogin_r(userName,sizeof(userName));
        std::string s(userName);
        // j'ai fini par organiser les fichiers GIS en deux tables ; une spécifique pour les cartes d'aptitudes
        for (std::string table : std::vector<std::string>{"layerApt","fichiersGIS"})
        {
            if (s=="lisein"){
                SQLstring="SELECT Code,Dir2,Nom,Type,NomComplet,Categorie,TypeVar, expert, visu, stat,NomCourt FROM "+table+";";
            } else {
                SQLstring="SELECT Code,Dir,Nom,Type,NomComplet,Categorie,TypeVar, expert, visu, stat ,NomCourt FROM "+table+";";
            }
            //std::cout << SQLstring << std::endl;

            sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                    std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                    std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                    std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                    Dico_GISfile.emplace(std::make_pair(aA,aB+"/"+aC));
                    Dico_RasterTable.emplace(std::make_pair(aA,table));
                    if ( sqlite3_column_type(stmt, 3)!=SQLITE_NULL && sqlite3_column_type(stmt, 4)!=SQLITE_NULL && sqlite3_column_type(stmt, 5)!=SQLITE_NULL && sqlite3_column_type(stmt, 6)!=SQLITE_NULL&& sqlite3_column_type(stmt, 10)!=SQLITE_NULL){
                        std::string aD=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
                        std::string aE=std::string( (char *)sqlite3_column_text( stmt, 4 ) );
                        std::string aF=std::string( (char *)sqlite3_column_text( stmt, 5 ) );
                        std::string aG=std::string( (char *)sqlite3_column_text( stmt, 6 ) );
                        std::string aH=std::string( (char *)sqlite3_column_text( stmt, 10 ) );
                        Dico_RasterType.emplace(std::make_pair(aA,aD));
                        Dico_RasterNomComplet.emplace(std::make_pair(aA,aE));
                        Dico_RasterNomCourt.emplace(std::make_pair(aA,aH));
                        Dico_RasterCategorie.emplace(std::make_pair(aA,aF));
                        Dico_RasterVar.emplace(std::make_pair(aA,aG));
                        bool expert(0);
                        if (sqlite3_column_type(stmt, 7)!=SQLITE_NULL) {expert=sqlite3_column_int( stmt, 7 );}
                        Dico_RasterExpert.emplace(std::make_pair(aA,expert));
                        if (sqlite3_column_type(stmt, 8)!=SQLITE_NULL) { Dico_RasterVisu.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 8 )));;}
                        if (sqlite3_column_type(stmt, 9)!=SQLITE_NULL) { Dico_RasterStat.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 9 )));;}
                    }
                }
            }
            sqlite3_finalize(stmt);
            SQLstring="SELECT Code,WMSurl,WMSlayer, WMSattribution, typeGeoservice FROM "+table+" WHERE WMSurl IS NOT NULL;";

            sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                    std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                    std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                    std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                    std::string attribution("Gembloux Agro-Bio Tech");
                    if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL) {attribution=std::string( (char *)sqlite3_column_text(stmt, 3 ));}
                    std::string geoservice("");
                    if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL){geoservice=std::string( (char *)sqlite3_column_text(stmt, 4 ));}

                    Dico_WMS.emplace(std::make_pair(aA,WMSinfo(aB,aC,geoservice,attribution)));
                }
            }
            sqlite3_finalize(stmt);
        }
        SQLstring="SELECT Code, id_projet, description, version, id_reference, Nom, copyrigth,ordre, NomShort FROM carteMTD;";

        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 7)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                LayerMTD lMTD;
                if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL ){lMTD.setDescr(std::string( (char *)sqlite3_column_text( stmt, 2 ) ));}
                if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL ){lMTD.setVersion(std::string( (char *)sqlite3_column_text( stmt, 3 ) ));}
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
                if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL ){
                    // le champ id est une liste de numéro sépraré par une virgule, il faut parser
                    std::vector<std::string> aVidRef;
                    std::string aListRef=std::string( (char *)sqlite3_column_text( stmt, 4 ));
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
                if (sqlite3_column_type(stmt, 5)!=SQLITE_NULL ){lMTD.setNom(std::string( (char *)sqlite3_column_text( stmt, 5 ) ));}
                if (sqlite3_column_type(stmt, 6)!=SQLITE_NULL ){lMTD.setCopyR(std::string( (char *)sqlite3_column_text( stmt, 6 ) ));}
                //std::cout << " done layerMTD " << lMTD.Nom() << std::endl;
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 7 ) );
                if (sqlite3_column_type(stmt, 8)!=SQLITE_NULL ){lMTD.setLabel(std::string( (char *)sqlite3_column_text( stmt, 8 ) ));}
                Dico_layerMTD.emplace(std::make_pair(aB,lMTD));
            }
        }

        sqlite3_finalize(stmt);
        SQLstring="SELECT Col,R,G,B FROM dico_color;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL&& sqlite3_column_type(stmt, 3)!=SQLITE_NULL){

                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                int R=sqlite3_column_int( stmt, 1 );
                int G=sqlite3_column_int( stmt, 2 );
                int B=sqlite3_column_int( stmt, 3 );
                colors.emplace(std::make_pair(aA,color(R,G,B,aA)));
            }
        }
        sqlite3_finalize(stmt);
        // palette des gris faite maison
        SQLstring="SELECT Col,R,G,B FROM dico_colGrey;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL&& sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                int R=sqlite3_column_int( stmt, 1 );
                int G=sqlite3_column_int( stmt, 2 );
                int B=sqlite3_column_int( stmt, 3 );
                colors.emplace(std::make_pair(aA,color(R,G,B,aA)));
            }
        }
        sqlite3_finalize(stmt);

        // par après je décide de travailler en code hex pour pas me faire ch***
        SQLstring="SELECT id,hex FROM dico_viridisColors;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                //std::cout << aA << ";";
                colors.emplace(std::make_pair(std::to_string(aA),color(aB,std::to_string(aA))));
            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT Code_Aptitude,Num,Equiv2Code,OrdreContrainte,Aptitude,col,surcote,souscote,EquCodeNonContr FROM dico_apt;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                int aB=sqlite3_column_int( stmt, 1 );
                int aC=sqlite3_column_int( stmt, 2 );
                int aD=sqlite3_column_int( stmt, 3 );
                std::string aE=std::string( (char *)sqlite3_column_text( stmt, 4 ) );
                std::string aF=std::string( (char *)sqlite3_column_text( stmt, 5 ) );
                int aG=sqlite3_column_int( stmt, 6 );
                int aH=sqlite3_column_int( stmt, 7 );
                Dico_Apt.emplace(std::make_pair(aA,aB));
                Dico_code2Apt.emplace(std::make_pair(aB,aA));
                Dico_AptDouble2AptContr.emplace(std::make_pair(aB,aC));
                Dico_Apt2OrdreContr.emplace(std::make_pair(aB,aD));
                Dico_code2AptFull.emplace(std::make_pair(aB,aE));
                Dico_AptFull2AptAcro.emplace(std::make_pair(aE,aA));
                Dico_codeApt2col.emplace(std::make_pair(aB,getColor(aF)));

                Dico_AptSurcote.emplace(std::make_pair(aB,aG));
                Dico_AptSouscote.emplace(std::make_pair(aB,aH));
                int aI=sqlite3_column_int( stmt, 8 );
                Dico_AptDouble2AptNonContr.emplace(std::make_pair(aB,aI));
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
        SQLstring="SELECT code,risque,categorie FROM dico_risque;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
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
                            colors.emplace(std::make_pair(col,color(col)));
                        }
                    }
                }
                sqlite3_finalize(stmt2);
            }
        }
        sqlite3_finalize(stmt);


        // toutes les essences de la classe essence
        for (auto & pair : *codeEs2Nom()){
            mVEss.emplace(std::make_pair(pair.first,std::make_shared<cEss>(pair.first,this)));
        }
        // toutes les layerbase
        for (auto & pair : Dico_RasterType){
            std::shared_ptr<layerBase> l=std::make_shared<layerBase>(pair.first,this);
            if (l->getCatLayer()!=TypeLayer::Externe & !l->rasterExist()){
                std::cout << " \n Attention layerBase " << l->Code() << ", fichier " << l->getPathTif() << " inexistant" << std::endl;
            }

            mVlayerBase.emplace(std::make_pair(pair.first,l));
        }
        closeConnection();
    }

    std::cout << "done " << std::endl;
    //std::cout << "Dico code essence --> nom essence francais a "<< Dico_codeEs2NomFR.size() << " elements \n" << std::endl;
    //std::cout << "Dico gis file a "<< Dico_GISfile.size() << " elements \n" << std::endl;
}

std::map<int,std::map<std::string,int>> cDicoApt::getFEEApt(std::string aCodeEs){
    std::map<int,std::map<std::string,int>> aRes;

    for (int i(1);i<11;i++){
        aRes.emplace(std::make_pair(i,std::map<std::string,int>()));
    }

    sqlite3_stmt * stmt=NULL;
    std::string SQLstring="SELECT CodeNTNH,'1','2','3','4','5','6','7','8','9','10' FROM AptFEE WHERE CODE_ESSENCE='"+ aCodeEs+"';";
    boost::replace_all(SQLstring, "'", "\"");
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
            int code=sqlite3_column_int( stmt, 0 );
            for (int i(1);i<11;i++){
                std::string apt=std::string( (char *)sqlite3_column_text( stmt, i ) );
                // i == zone bioclim
                std::string codeNTNH=code2NTNH(code);
                int codeApt=Apt(apt);
                // fuite de mémoire possible avec cette commande
                //aRes[i].emplace(std::make_pair(codeNTNH,codeApt));
                aRes.at(i).insert(std::make_pair(codeNTNH,codeApt));
            }
        }
    }
    sqlite3_finalize(stmt);
    return aRes;
}

std::map<int,int> cDicoApt::getZBIOApt(std::string aCodeEs){
    std::map<int,int> aRes;
    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT '1','2','3','4','5','6','7','8','9','10' FROM AptFEE_ZBIO WHERE CODE_ESSENCE='"+ aCodeEs+"';";
    boost::replace_all(SQLstring, "'", "\"");
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
    sqlite3_finalize(stmt);
    return aRes;
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
    SQLstring="SELECT "+field_raster+", col FROM "+ nom_dico ;
    if (cond!=""){ SQLstring=SQLstring+" WHERE "+cond+";";} else {SQLstring=SQLstring+";";}
    //std::cout << SQLstring << "\n\n" << std::endl;
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){ // attention, la colonne col peut être vide!! ha ben non plus maintenant.
            int aA=sqlite3_column_int( stmt, 0 );
            std::string aB("");
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL ) {aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );}

            // if (aCode=="MF"){ std::cout << " ajout dans dicoCol " << aA << " , col " << aB << std::endl;}
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


std::map<int,std::map<int,int>> cDicoApt::getCSApt(std::string aCodeEs){
    std::map<int,std::map<int,int>> aRes;

    sqlite3_stmt * stmt;
    // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
    for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
        std::string SQLstring="SELECT stat_id,"+aCodeEs+" FROM AptCS WHERE ZBIO="+ std::to_string(zbio)+";";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
    }
    sqlite3_finalize(stmt);

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

    return aRes;
}

std::map<int,std::map<int,int>> cDicoApt::getRisqueTopo(std::string aCodeEs){
    std::map<int,std::map<int,int>> aRes;

    sqlite3_stmt * stmt;
    // hors Ardenne
    std::string SQLstring="SELECT Secteurfroid,Secteurneutre,Secteurchaud,fond_Vallee FROM Risque_topoFEE WHERE Code_Fr='"+ aCodeEs+"';";
    boost::replace_all(SQLstring, "'", "\"");
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
    sqlite3_finalize(stmt);
    sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
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
    return aRes;
}

int cDicoApt::openConnection(){
    int rc;

    std::cout << "ouvre connexion avec BD dictionnaire ..." << std::endl;
    //db_->Sqlite3(mBDpath);
    rc = sqlite3_open_v2(mBDpath.c_str(), db_,SQLITE_OPEN_READONLY,NULL);
    // The 31 result codes are defined in sqlite3.h
    //SQLITE_ERROR (1)
    if( rc!=0) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db_));
        std::cout << " mBDpath " << mBDpath << std::endl;
        std::cout << "result code : " << rc << std::endl;
    }
    //SQLITE_BUSY (5)
    //SQLITE_CANTOPEN (14)
    //SQLITE_OK (0)

    //std::cout << std::endl << " bd ouverte..., rc = " << rc << std::endl;;

    return rc;
}

void cDicoApt::closeConnection(){

    int rc = sqlite3_close_v2(*db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(*db_));
    }
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

cEss::cEss(std::string aCodeEs,cDicoApt * aDico):mCode(aCodeEs),mNomFR(aDico->accroEss2Nom(aCodeEs)),mDico(aDico)
  ,mType(Apt),mPrefix(aDico->accroEss2prefix(aCodeEs)){
    //std::cout << "creation de l'essence " << mNomFR << std::endl;
    mEcoVal=aDico->getFEEApt(mCode);
    mAptCS=aDico->getCSApt(mCode);
    mAptZbio=aDico->getZBIOApt(mCode);
    mRisqueTopo=aDico->getRisqueTopo(mCode);
}

//effectue la confrontation Apt Zbio et AptHydroTrophiue
int cEss::getApt(int aCodeNT, int aCodeNH, int aZbio, bool hierachique){
    int aRes(12); // indéterminé zone batie ; par défaut
    if (aCodeNT==0 && aCodeNH!=0){aRes=11;}
    if (mAptZbio.find(aZbio)==mAptZbio.end()){aRes=11;}// hors belgique ; Indéterminé mais pas zone batie

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
    // attention, si pas d'aptitude hydro-trophique, aRes
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


std::string cEss::NomCarteAptFEE(){return mDico->File("OUTDIR")+"aptitudeFEE_"+mCode+".tif";}
//std::string cEss::NomDirTuileAptFEE(){return mDico->File("OUTDIR2")+"FEE_"+mCode;}
std::string cEss::shortNomCarteAptFEE(){return "aptitudeFEE_"+mCode+".tif";}
std::string cEss::NomCarteAptCS(){return mDico->File("OUTDIR")+"aptitudeCS_"+mCode+".tif";}
//std::string cEss::NomDirTuileAptCS(){return mDico->File("OUTDIR2")+"CS_"+mCode;}
std::string cEss::shortNomCarteAptCS(){return "aptitudeCS_"+mCode+".tif";}

std::string cEss::NomMapServerLayer(){return "Aptitude_FEE_"+mCode;}
std::string cEss::NomMapServerLayerFull(){return "Aptitude "+mPrefix+mNomFR;}

std::string cEss::NomMapServerLayerCS(){return "Aptitude_CS_"+mCode;}

bool cEss::hasRisqueComp(int zbio,int topo){
    bool aRes(0);
    int risque=getRisque(zbio,topo);
    int catRisque=mDico->risqueCat(risque);
    if ((catRisque==1) | (catRisque==3)){aRes=1;}
    return aRes;
}

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
    return aRes;
}

TypeWMS str2TypeWMS(const std::string& str){
    TypeWMS aRes=TypeWMS::WMS;
    if(str == "ArcGisRest") {aRes=TypeWMS::ArcGisRest;}
    return aRes;
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
