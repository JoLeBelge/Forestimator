#include "cdicoaptbase.h"

cdicoAptBase::cdicoAptBase(std::string aBDFile):mBDpath(aBDFile),ptDb_(NULL)
{
    db_=&ptDb_;
    //*db_=NULL;
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

        std::cout << "close connection (dicoAptBase)" << std::endl;
        closeConnection();

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

    std::cout << "ouvre connexion avec BD dictionnaire ..." << std::endl;
    //db_->Sqlite3(mBDpath);
    //rc = sqlite3_open_v2(mBDpath.c_str(), db_,SQLITE_OPEN_READONLY,NULL);
    rc = sqlite3_open(mBDpath.c_str(), db_);
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

std::string roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {
        aRes=std::to_string(d+0.5).substr(0, std::to_string(d+0.5).find("."));}
    return aRes;
}

