#include "cdicoaptbase.h"

std::string columnPath("Dir2");
extern bool globTest;

cdicoAptBase::cdicoAptBase(std::string aBDFile):mBDpath(aBDFile),ptDb_(NULL)
{
    db_=&ptDb_;

    if (openConnection()){
    std::cout << " bd pas ouverte!!!\n"<< std::endl;
    } else {
        // dico Ess Nom -- code
        sqlite3_stmt * stmt;

        std::string SQLstring="SELECT Ess_FR,Code_FR,prefix, FeRe FROM dico_essences ORDER BY Ess_FR DESC;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aNomEs=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aCodeEs=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aPrefix=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_codeEs2NomFR.emplace(std::make_pair(aCodeEs,aNomEs));
                Dico_code2prefix.emplace(std::make_pair(aCodeEs,aPrefix));
                int FR=sqlite3_column_int( stmt, 3 );
                Dico_F_R.emplace(std::make_pair(aCodeEs,FR));
                Dico_Ess.push_back(aCodeEs);
            }
        }

        sqlite3_finalize(stmt);

        SQLstring="SELECT ZBIO,stat_id,Station_carto,var,nom_var,varMajoritaire, stat_num FROM dico_station;"; //WHERE stat_id=
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                std::string aD(""),aF("");
                int varMaj(0);
                if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
                aD=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
                }
                if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL){
                aF=std::string( (char *)sqlite3_column_text( stmt, 4 ) );
                }
                if (sqlite3_column_type(stmt, 5)!=SQLITE_NULL){
                varMaj=sqlite3_column_int( stmt, 5 ) ;
                }
                int aG=sqlite3_column_int( stmt, 6 );
                Dico_station[aA].emplace(std::make_pair(std::make_tuple(aB,aD),aC));
                Dico_station_varName[aA].emplace(std::make_pair(std::make_tuple(aB,aD),aF));
                if (varMaj==1){
                   Dico_station_varMaj[aA].emplace(std::make_pair(aB,aD));
                }
                Dico_station_statNum[aA].emplace(std::make_pair(aG,aB));

                //std::cout << "zbio " << aA << " station " << aB << ", variante " << aD << std::endl;
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT Nom,Zbio,CS_lay, CSid FROM dico_zbio;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                int aB=sqlite3_column_int( stmt, 1 );

                if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                int aD=sqlite3_column_int( stmt, 3 );
                Dico_ZBIO2layCS.emplace(std::make_pair(aB,aC));
                Dico_ZBIO2CSid.emplace(std::make_pair(aB,aD));
                }

                Dico_ZBIO.emplace(std::make_pair(aB,aA));

            }
        }
        sqlite3_finalize(stmt);
        SQLstring="SELECT raster_val,NH,posEco, id_groupNH2 FROM dico_raster_nh;";
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
        SQLstring="SELECT raster_val,NT,posEco,label FROM dico_raster_nt;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                int aB=sqlite3_column_int( stmt, 0 );
                Dico_NT.emplace(std::make_pair(aB,aA));
                if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){ Dico_NTposEco.emplace(std::make_pair(aB,sqlite3_column_int( stmt, 2 )));}
                if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL){ Dico_NTLabel.emplace(std::make_pair(aB,std::string( (char *)sqlite3_column_text( stmt, 3 ))));}
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

        std::string sqlString2;
        // j'ai fini par organiser les fichiers GIS en deux tables ; une spécifique pour les cartes d'aptitudes
        for (std::string table : std::vector<std::string>{"layerApt","fichiersGIS"})
        {

            SQLstring="SELECT Code,"+columnPath+",Nom,Type,NomComplet,Categorie,TypeVar, expert, visu, stat,NomCourt,groupe,statPonct, res FROM "+table+";";
            sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                    std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                    std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                    std::string aC("");
                    if(sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                        aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                    }
                    Dico_GISfile.emplace(std::make_pair(aA,aB+"/"+aC));
                    Dico_RasterTable.emplace(std::make_pair(aA,table));
                    std::string groupe("REF");
                    if (sqlite3_column_type(stmt, 11)!=SQLITE_NULL) {groupe=std::string( (char *)sqlite3_column_text( stmt, 11 ) );}
                    Dico_lay2groupe.emplace(std::make_pair(aA,groupe));

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
                        if (sqlite3_column_type(stmt, 8)!=SQLITE_NULL) { Dico_RasterVisu.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 8 )));}
                        if (sqlite3_column_type(stmt, 9)!=SQLITE_NULL) { Dico_RasterStat.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 9 )));}

                        if (sqlite3_column_type(stmt, 12)!=SQLITE_NULL) { Dico_RasterStatP.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 12 )));}
                        if (sqlite3_column_type(stmt, 13)!=SQLITE_NULL) { Dico_RasterResolution.emplace(std::make_pair(aA,sqlite3_column_double( stmt, 13 )));}
                    }
                }
            }
            if(Dico_GISfile.size()==0) { std::cout << "\n Il se pourrait que la colonne " << columnPath << " n'existe pas dans la table fichiersGIS " << std::endl;}

                for (auto kv : Dico_GISfile){
                    if(!boost::filesystem::exists(kv.second)){
                        //std::cout << " fichier " << kv.second << " n'existe pas " << std::endl;
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

            sqlString2="SELECT gain, Code FROM "+table+" WHERE gain IS NOT NULL;";
            sqlite3_prepare_v2( *db_, sqlString2.c_str(), -1, &stmt, NULL );
            while(sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
                    double aA=sqlite3_column_double(stmt, 0 );
                    std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                    Dico_RasterGain.emplace(std::make_pair(aB,aA));
                }
            }
            sqlite3_finalize(stmt);


        }


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
                Dico_AptSurcote.emplace(std::make_pair(aB,aG));
                Dico_AptSouscote.emplace(std::make_pair(aB,aH));
                int aI=sqlite3_column_int( stmt, 8 );
                Dico_AptDouble2AptNonContr.emplace(std::make_pair(aB,aI));
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
                colors.emplace(std::make_pair(aA,std::make_shared<color>(R,G,B,aA)));
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
                colors.emplace(std::make_pair(aA,std::make_shared<color>(R,G,B,aA)));
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
                colors.emplace(std::make_pair(std::to_string(aA),std::make_shared<color>(aB,std::to_string(aA))));
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
        SQLstring="SELECT id,code_nh_start,nb_nh,label FROM dico_groupeNH2;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL && sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                dico_groupeNH2Label.emplace(std::make_pair(aA,std::string( (char *)sqlite3_column_text( stmt, 3 ))));
                dico_groupeNH2Nb.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 2 )));
                dico_groupeNH2NHStart.emplace(std::make_pair(aA,sqlite3_column_int( stmt, 1 )));
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

        SQLstring="SELECT code,val FROM dico_caracteristiqueCS;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                Dico_codeKK2Nom.emplace(std::make_pair(aA,aB));
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT code_exact,noms_complets,raster_val from dico_habitats;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                int aC=sqlite3_column_int( stmt, 2 );
                //Dico_code2NomHabitat.emplace(std::make_pair(aA,aB));
                Dico_code2rasterValHabitat.emplace(std::make_pair(aA,aC));
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT raster_val,label from dico_recommandation;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ));
                int aA=sqlite3_column_int( stmt, 0 );
                Dico_code2Recommandation.emplace(std::make_pair(aA,aB));
            }
        }
        sqlite3_finalize(stmt);

        SQLstring="SELECT label,code,col from dico_CSClim;";
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ));
                int aB=sqlite3_column_int( stmt,1 );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ));
                Dico_CSClim.emplace(std::make_pair(aB,aA));
                Dico_CSClim2col.emplace(std::make_pair(aB,aC));
            }
        }
        sqlite3_finalize(stmt);

        // toutes les essences de la classe essence
        for (auto & pair : *codeEs2Nom()){
            mVEss.emplace(std::make_pair(pair.first,std::make_shared<cEss>(pair.first,this)));
        }

        std::cout << "close connection (dicoAptBase)" << std::endl;
        closeConnection();

        // lecture de table avec dbo
        std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3(mBDpath)};
        Wt::Dbo::Session session;
        session.setConnection(std::move(sqlite3));
        session.mapClass<caracteristiqueCS>("caracteristiqueCS");
        dbo::Transaction transaction{session};
        // je met tout ça dans une map
        //typedef dbo::collection< dbo::ptr<caracteristiqueCS> > collectionKKCS;
        //collectionKKCS col = session.find<caracteristiqueCS>().where("zbio = ?").bind(1);
        // les collection  de type dbo ne fonctionnent pas pour moi, en tout cas quand j'effectue l'itération avec un for (auto : ), c'est toujours le mm élément qu'il me retourne size() fois.
        //dbo::collection<dbo::ptr<caracteristiqueCS>>::const_iterator i = col.begin();
        //while(i != col.end())
        /*{
        std::cout << "station id: " << (*i)->station_id;
        i++;
        }*/
        for (int us(1);us <18;us++){
           dbo::ptr<caracteristiqueCS>  pt = session.find<caracteristiqueCS>().where("zbio = ?").bind(1).where("station_id = ?").bind(us);
           caracteristiqueCS  kkCSCopy(pt.get());
         //  if (globTest){std::cout <<"encore une caracteristiqueCS " << kkCSCopy.station_id << " , " << kkCSCopy.zbio <<  std::endl;}
           Dico_US2KK.emplace(std::make_pair(std::make_pair(kkCSCopy.zbio,kkCSCopy.station_id),kkCSCopy));
        }

    }

    //std::cout << "dicoAptBase done " << std::endl;
}

void cdicoAptBase::closeConnection(){

    //int rc = sqlite3_close_v2(*db_);
    int rc = sqlite3_close(*db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(*db_));
    }
}

int cdicoAptBase::openConnection(){
    int rc;

    std::cout << "ouvre connexion avec BD dictionnaire ... avec colonne " << columnPath << " pour chemin access aux fichiers" << std::endl;
    rc = sqlite3_open(mBDpath.c_str(), db_);
    // The 31 result codes are defined in sqlite3.h

    if( rc!=0) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*db_));
        std::cout << " mBDpath " << mBDpath << std::endl;
        std::cout << "result code : " << rc << std::endl;
    }

    return rc;
}


std::map<int,std::map<std::string,int>> cdicoAptBase::getFEEApt(std::string aCodeEs){
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

std::map<int,int> cdicoAptBase::getZBIOApt(std::string aCodeEs){
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


std::map<int,std::map<int,int>> cdicoAptBase::getRisqueTopo(std::string aCodeEs){
    std::map<int,std::map<int,int>> aRes;

    sqlite3_stmt * stmt;
    // hors Ardenne
    std::string SQLstring="SELECT Secteurfroid,Secteurneutre,Secteurchaud,Fond_vallee FROM Risque_topoFEE WHERE Code_Fr='"+ aCodeEs+"';";
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


std::map<int,std::map<std::tuple<int, std::string>,int>> cdicoAptBase::getCSApt(std::string aCodeEs){
    std::map<int,std::map<std::tuple<int, std::string>,int>> aRes;

    sqlite3_stmt * stmt;
    // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
    for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
        std::string SQLstring="SELECT stat_id,"+aCodeEs+",var FROM AptCS WHERE ZBIO="+ std::to_string(zbio)+";";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int station=sqlite3_column_int( stmt, 0 );
                std::string apt=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                 std::string var="";
                if(sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                var=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                }
                int codeApt=Apt(apt);
                aRes[zbio].emplace(std::make_pair(std::make_tuple(station,var),codeApt));
            }
        }
        sqlite3_finalize(stmt);
    }
    return aRes;
}

std::map<int,std::map<std::tuple<int, std::string>,int>> cdicoAptBase::getCSRisqueClim(std::string aCodeEs){
    std::map<int,std::map<std::tuple<int, std::string>,int>> aRes;

    sqlite3_stmt * stmt;
    // boucle sur tout les identifiants de zbio mais attention, les catalogues de station ne couvrent pas tout donc vérif si ND
    for(auto&& zbio : Dico_ZBIO | boost::adaptors::map_keys){
        std::string SQLstring="SELECT stat_id,"+aCodeEs+",var FROM AptCSClim WHERE ZBIO="+ std::to_string(zbio)+";";
        //std::cout << SQLstring << std::endl;
        sqlite3_prepare_v2( *db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int station=sqlite3_column_int( stmt, 0 );
                int clim=sqlite3_column_int( stmt, 1 );
                 std::string var="";
                if(sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                var=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                }
                aRes[zbio].emplace(std::make_pair(std::make_tuple(station,var),clim));
            }
        }
        sqlite3_finalize(stmt);
    }
    return aRes;
}

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){
        aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);
    }
    else  {
        aRes=std::to_string(d+0.5).substr(0, std::to_string(d+0.5).find("."));
    }
    return aRes;
}

cEss::cEss(std::string aCodeEs, cdicoAptBase *aDico):mCode(aCodeEs),mNomFR(aDico->accroEss2Nom(aCodeEs)),mDico(aDico)
  ,mPrefix(aDico->accroEss2prefix(aCodeEs)){ //,mType(Apt)
    //std::cout << "creation de l'essence " << mNomFR << std::endl;
    mEcoVal = aDico->getFEEApt(mCode);
    mAptCS = aDico->getCSApt(mCode);
    mCSClim = aDico->getCSRisqueClim(mCode);
    mAptZbio = aDico->getZBIOApt(mCode);
    mRisqueTopo = aDico->getRisqueTopo(mCode);
    mFeRe = aDico->accroEss2FeRe(aCodeEs);
}

//effectue la confrontation Apt Zbio et AptHydroTrophiue
int cEss::getApt(int aCodeNT, int aCodeNH, int aZbio, bool hierachique,int aTopo){
    int aRes(12); // indéterminé zone batie ; par défaut
    if (aCodeNT==0 && aCodeNH!=0){aRes=11; return aRes;}
    if (mAptZbio.find(aZbio)==mAptZbio.end()){aRes=11;return aRes;}// hors belgique ; Indéterminé mais pas zone batie

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

        if (aTopo!=666){
            // c'est sur cette aptitude que l'on applique un facteur de correction
            aZbioApt=corrigAptBioRisqueTopo(aZbioApt,aTopo,aZbio);
        }

        if (mDico->AptContraignante(aRes)<mDico->AptContraignante(aZbioApt)){
            aRes=aZbioApt;
        }
    }
    return aRes;
}

int cEss::getApt(int aZbio, int US, std::string aVar, bool withClim){
    int aRes(0);
    // on prend par défaut la station majoritaire
    std::string var=mDico->getStationMaj(aZbio,US);   
    std::tuple<int,std::string> aUSkey=std::make_tuple(US,var);

    if (mAptCS.find(aZbio)!=mAptCS.end()){
        std::map<std::tuple<int, std::string>,int> * Apt=&mAptCS.at(aZbio);

        if (Apt->find(aUSkey)!=Apt->end()){
            aRes=Apt->at(aUSkey);
        }
        // si l'utilisateur a renseigné une variante avec l'argument aVar:
        if (Apt->find(std::make_tuple(US,aVar))!=Apt->end()){
            aRes=Apt->at(std::make_tuple(US,aVar));
        }
    }


    if (aRes>9){aRes=0;}

    // combinaison recommandation avec risque climatique (4 classes x 4 classes)
    if (withClim){
        int aClim(0);

        aClim=getCSClim(aZbio,US,aVar);
        //std::cout << "risque climatique pour zbio " << aZbio << " US " << US << " et variante " << aVar << " : " <<aClim << std::endl;
        /*std::map<std::tuple<int, std::string>,int> * clim=& mCSClim.at(2);

        if (clim->find(aUSkey)!=clim->end()){
            aClim=clim->at(aUSkey);
        }
        // si l'utilisateur a renseigné une variante avec l'argument aVar:
        if (clim->find(std::make_tuple(US,aVar))!=clim->end()){
            aClim=clim->at(std::make_tuple(US,aVar));
        }*/

        // on combine recommandation avec clim - cela donne de 0 à 13
        if (aRes==4){aRes=13;} else if (aRes!=0){
        aRes=(aRes-1)*4+aClim;}
        // maintenant on regroupe certaine classes ensemble
        std::map<int,int> lut = {{0,0},{1, 1}, {2, 1}, {3, 3}, {4, 5},{5, 2},{6, 2},{7, 3},{8, 6},{9, 2},{10, 4},{11, 4},{12, 4},{13, 7}};
        if (lut.find(aRes)==lut.end()){std::cout << "attention, aptitude avec sensibilité bioclim de " << aRes << std::endl;
        aRes=7;} else{
        aRes=lut.at(aRes);
        }
    }
    return aRes;
}

int cEss::getCSClim(int zbio, int US, std::string aVar){
    int aRes(0);
    if (zbio==1){zbio=2;}
    if (mCSClim.find(zbio)!=mCSClim.end()){
        std::map<std::tuple<int, std::string>,int> * clim=&mCSClim.at(zbio);
        // on prend par défaut la station majoritaire
        std::string var=mDico->getStationMaj(2,US);
        std::tuple<int,std::string> aUSkey=std::make_tuple(US,var);
        if (clim->find(aUSkey)!=clim->end()){
            aRes=clim->at(aUSkey);
        }

        // si l'utilisateur a renseigné une variante avec l'argument aVar:
        if (clim->find(std::make_tuple(US,aVar))!=clim->end()){
            aRes=clim->at(std::make_tuple(US,aVar));
        }
    }
    //if (aRes>9){aRes=0;}
    return aRes;
}

int cEss::getApt(int aZbio){
    int aRes(0);
    if (mAptZbio.find(aZbio)!=mAptZbio.end()){
        aRes=mAptZbio.at(aZbio);
    }
    return aRes;
}

// octobre 2021 ; la compensation topo s'applique exclusivement sur l'aptitude bioclim,
int cEss::corrigAptBioRisqueTopo(int aptBio,int topo,int zbio){
    int aRes=aptBio;
    int risque=getRisque(zbio,topo);
    int catRisque=mDico->risqueCat(risque);
    // situation favorable
    if (catRisque==1){
        aRes=mDico->AptSurcote(aptBio);
        return aRes;
        //std::cout << "Surcote l'aptitude " << mDico->code2Apt(apt) << " vers " << mDico->code2Apt(aRes) << std::endl;
    }
    // risque élevé et très élevé
    if (catRisque==3){aRes=mDico->AptSouscote(aptBio);}
    // attention, pour résineux, pas de Tolérance Elargie --> exclusion
    if (aRes==3 && mFeRe==FeRe::Resineux){aRes=4;}
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

bool cEss::hasRisqueComp(int zbio,int topo){
    bool aRes(0);
    int risque=getRisque(zbio,topo);
    int catRisque=mDico->risqueCat(risque);
    if ((catRisque==1) | (catRisque==3)){aRes=1;}
    return aRes;
}

TypeWMS str2TypeWMS(const std::string& str){
    TypeWMS aRes=TypeWMS::WMS;
    if(str == "ArcGisRest") {aRes=TypeWMS::ArcGisRest;}
    return aRes;
}

std::shared_ptr<cEss> cdicoAptBase::getEss(std::string aCode){
    std::shared_ptr<cEss> aRes=NULL;
    if (hasEss(aCode)){aRes=mVEss.at(aCode);} else {
        std::cout << "getEss de cdicoapt, création d'une essence vide pour " << aCode << ", attention " << std::endl;
        aRes= std::make_shared<cEss>("toto",this);
    }
    return aRes;
}
