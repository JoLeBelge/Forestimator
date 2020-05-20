#include "cdicocarteph.h"


cDicoCartepH::cDicoCartepH(std::string aBDFile):mBDpath(aBDFile)
{

    int rc;
    std::cout << "chargement des dictionnaires de la BD carte pH ..." ;

    rc = sqlite3_open_v2(mBDpath.c_str(), &db_,SQLITE_OPEN_READONLY,NULL);
    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
        std::cout << " mBDpath " << mBDpath << std::endl;
    } else {

        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT Zbio,RN FROM dico_zbio;";
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                Dico_Zbio2RN.emplace(std::make_pair(aA,aB));
            }
        }
        SQLstring="SELECT ID,Nom FROM dico_regnat;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                Dico_codeRN2Nom.emplace(std::make_pair(aA,aB));
                Dico_NomRN2Code.emplace(std::make_pair(aB,aA));
            }
        }

        SQLstring="SELECT ID,Nom FROM dico_pts;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                Dico_PTS.emplace(std::make_pair(aA,aB));
            }
        }

        SQLstring="SELECT PTS,pH_moy FROM pH_pts;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                std::string aBtmp=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                double aB=toDouble(aBtmp);

                std::cout << "ph pour PTS " << aA << " est de " << aB << std::endl;
                Dico_PTS2pH.emplace(std::make_pair(aA,aB));
            }
        }

        SQLstring="SELECT PTS,RegNat,pH_moy FROM pH_regnat_pts;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aCtmp=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                double aC=toDouble(aCtmp);

                std::vector<int> aV{aA,Dico_NomRN2Code.at(aB)};
                Dico_PTSetRN2pH.emplace(std::make_pair(aV,aC));
            }
        }

        SQLstring="SELECT Code,Dir,Nom FROM fichiersGIS;";

        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aC=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                Dico_GISfile.emplace(std::make_pair(aA,aB+"/"+aC));
            }
        }

        sqlite3_finalize(stmt);
    }

    std::cout << "\nDico pts to ph  a "<< Dico_PTS.size() << " elements" << std::endl;
    std::cout << "\nDico pts;regnat to ph  a "<< Dico_PTSetRN2pH.size() << " elements" << std::endl;
    std::cout << "done" << std::endl ;
    closeConnection();
}

void cDicoCartepH::closeConnection(){

    int rc = sqlite3_close_v2(db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(db_));
    }
}

double cDicoCartepH::getpH(int aPTS,int aZBIO){
    double aRes(1.0);
    if (Dico_Zbio2RN.find(aZBIO)!=Dico_Zbio2RN.end()){
        int aRN=Dico_Zbio2RN.at(aZBIO);
        std::vector<int> aKey{aPTS,aRN};
        // on regarde pour commencer si on a une valeur de pH pour cette combinaison PTS-RN
        if (Dico_PTSetRN2pH.find(aKey)!=Dico_PTSetRN2pH.end()){aRes=Dico_PTSetRN2pH.at(aKey);
        } else {
            if (Dico_PTS2pH.find(aPTS)!=Dico_PTS2pH.end()){aRes=Dico_PTS2pH.at(aPTS);}
        }
    }
    return aRes;
}

double cDicoCartepH::toDouble(std::string aStr){
    boost::replace_all(aStr, ",", ".");
    return std::stod(aStr);
}
