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

        SQLstring="SELECT MAT_TEXT, Equiv FROM dico_MAT_TEXT;;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string aA=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                Dico_MAT_TEXT.emplace(std::make_pair(aA,aB));

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

                //std::cout << "ph pour PTS " << aA << " est de " << aB << std::endl;
                Dico_PTS2pH.emplace(std::make_pair(aA,aB));
            }
        }

        SQLstring="SELECT PTS,RegNat,pH_moy,pH_nb FROM pH_regnat_pts WHERE keep=1;";
        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                std::string aB=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string aCtmp=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                double aC=toDouble(aCtmp);


                 int aD=sqlite3_column_int( stmt, 3 );

                std::vector<int> aV{aA,Dico_NomRN2Code.at(aB)};
                Dico_PTSetRN2pH.emplace(std::make_pair(aV,aC));
                Dico_PTSetRN2NbpH.emplace(std::make_pair(aV,aD));
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

        SQLstring="SELECT INDEX_SOL,pts_typologie FROM dico_siglePedo_pts;";

        sqlite3_reset(stmt);
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){

                int aA=sqlite3_column_int( stmt, 0 );
                int aB=sqlite3_column_int( stmt, 1 );
                Dico_IndexSiglePed2PTS.emplace(std::make_pair(aA,aB));
            }
        }



        sqlite3_finalize(stmt);


    }

   // std::cout << "\nDico pts to ph  a "<< Dico_PTS.size() << " elements" << std::endl;
    //std::cout << "\nDico pts;regnat to ph  a "<< Dico_PTSetRN2pH.size() << " elements" << std::endl;

    closeConnection();

    // mapping de la classe siglePedo
    if (!boost::filesystem::exists(mBDpath)){std::cout << " bd pH" << mBDpath << " n'existe pas!! ça va planter ... \n\n\n\n" <<std::endl;}
    std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3(mBDpath)};

    session.setConnection(std::move(sqlite3));
    session.mapClass<siglePedo>("dico_cnsw");

    // j'ai essayer de créer une collection de siglePedo mais ça bug ; me crée la collection mais quand je boucle dessus, j'ai toujours le mm objet qui est l'objet de la première ligne en fait.

    //dbo::ptr<siglePedo> s1 = session.find<siglePedo>().where("INDEX_=?").bind("10");
    //s1->cat();

    //typedef dbo::collection< dbo::ptr<siglePedo> > collectionSigles;
    //dbo::Transaction transaction{session};
    //collectionSigles dico_cnsw = session.find<siglePedo>().where("INDEX_ IS NOT NULL");;
    //std::cout << "nombre de sigles pedo mappées : " << dico_cnsw.size() << std::endl;

    /*for (const dbo::ptr<siglePedo> &s : dico_cnsw){
        std::cout << "s " << s->getIndex() << std::endl;
        // s->cat();
        //mMSigles.emplace(s->getIndex(),s);
        if (s->getIndex()==10){s->cat();}

    }*/
     //std::cout << "nombre de sigles pedo mappées : " << dico_cnsw.size() << std::endl;
    //std::cout << "nombre de sigles dans la map : " << mMSigles.size() << std::endl;

    //Wt::Dbo::Query<dbo::ptr<siglePedo>> query {session.find<siglePedo>()};
    //Wt::Dbo::collection<dbo::ptr<siglePedo>> collection {query.resultList()};
    //std::vector<dbo::ptr<siglePedo>> vec; //{collection.begin(), collection.end()};
    //for (int i(1);i>)

    //std::cout << "vect has : " << vec.size() << std::endl;

     /*for (dbo::ptr<siglePedo> s : vec){
          s->cat();
     }*/

    std::cout << "done" << std::endl ;

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
        if (Dico_PTSetRN2pH.find(aKey)!=Dico_PTSetRN2pH.end() && Dico_PTSetRN2NbpH.at(aKey)>1){aRes=Dico_PTSetRN2pH.at(aKey);
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

std::vector<std::string> substratCalcaire{"j", "k", "m", "n", "i", "kf", "j-w", "ks", "kt", "(+u)"};
std::vector<std::string> PHASE_2Calcaire{"(ca)","(k)"};
std::vector<std::string> PHASE_6Calcaire{"J","M"};
std::vector<std::string> CHARGECalcaire{"k", "K", "kf", "m", "n"};
std::vector<std::string> SER_SPECRiche{"B", "R", "S", "J"};
std::vector<std::string> PHASE_1Profond{"0", "1", "0_1", "0_1_2"};
std::vector<std::string> PHASE_2Profond{"0", "1", "0_1", "0_1_2"};
std::vector<std::string> DEV_PROFILalluvion{"p", "P"};
std::vector<std::string> DEV_PROFILPodzolique{"c", "f"};
std::vector<std::string> MAT_TEXTtourbe{"V", "W", "V-E"};
std::vector<std::string> PHASE_4tourbe{"(v)","(v3)", "(v4)"};
std::vector<std::string> MAT_TEXTlimon{"A", "A-L", "A-E", "A-U"};

void siglePedo::prepare(){
    mCalcaire=0;
    bool t1 =std::find(substratCalcaire.begin(), substratCalcaire.end(), SUBSTRAT) !=substratCalcaire.end() ;
    bool t2 =std::find(PHASE_2Calcaire.begin(), PHASE_2Calcaire.end(), mPHASE_2) !=PHASE_2Calcaire.end() ;
    bool t3 =std::find(PHASE_6Calcaire.begin(), PHASE_6Calcaire.end(), mPHASE_6) !=PHASE_6Calcaire.end() ;
    bool t4 =std::find(CHARGECalcaire.begin(), CHARGECalcaire.end(), mCHARGE) !=CHARGECalcaire.end() ;
    if (t1 | t2 | t3 | t4){ mCalcaire=1;}

    mSsriche=0;
    t1 =std::find(SER_SPECRiche.begin(), SER_SPECRiche.end(), mSER_SPEC) !=SER_SPECRiche.end() ;
    if (t1){ mSsriche=1;}

    mProfond=0;
     t1 =std::find(PHASE_1Profond.begin(), PHASE_1Profond.end(), mPHASE_1) !=PHASE_1Profond.end() ;
    t2 =std::find(PHASE_2Profond.begin(), PHASE_2Profond.end(), mPHASE_2) !=PHASE_2Profond.end() ;
    if (t1 | t2){ mProfond=1;}
    if (mPHASE_1=="" && mPHASE_2==""){ mProfond=1;}

    mAlluvion=0;
    t1 =std::find(DEV_PROFILalluvion.begin(), DEV_PROFILalluvion.end(), mDEV_PROFIL) !=DEV_PROFILalluvion.end() ;
    if (t1){mAlluvion=1;}

    mPodzol=0;
    if (mDEV_PROFIL=="g"){mPodzol=1;}

    mPodzolique=0;
    t1 =std::find(DEV_PROFILPodzolique.begin(),DEV_PROFILPodzolique.end(), mDEV_PROFIL) !=DEV_PROFILPodzolique.end() ;
  if (t1){mPodzolique=1;}

    mSuperficiel=0;
     if (mPHASE_1=="6" | mPHASE_2=="6"){mSuperficiel=1;}

    mTourbe=0;
   t1 =std::find(MAT_TEXTtourbe.begin(),MAT_TEXTtourbe.end(), mMAT_TEXT) !=MAT_TEXTtourbe.end() ;
   t2 =std::find(PHASE_4tourbe.begin(),PHASE_4tourbe.end(), mPHASE_4) !=PHASE_4tourbe.end() ;
   if (t1 | t2){mTourbe=1;}

    mLimon=0;
    t1 =std::find(MAT_TEXTlimon.begin(),MAT_TEXTlimon.end(), mMAT_TEXT) !=MAT_TEXTlimon.end() ;
   if (t1){mLimon=1;}


}



/*
bool siglePedo::calcaire() const{
    bool aRes(0);
    bool t1 =std::find(substratCalcaire.begin(), substratCalcaire.end(), SUBSTRAT) !=substratCalcaire.end() ;
    bool t2 =std::find(PHASE_2Calcaire.begin(), PHASE_2Calcaire.end(), mPHASE_2) !=PHASE_2Calcaire.end() ;
    bool t3 =std::find(PHASE_6Calcaire.begin(), PHASE_6Calcaire.end(), mPHASE_6) !=PHASE_6Calcaire.end() ;
    bool t4 =std::find(CHARGECalcaire.begin(), CHARGECalcaire.end(), mCHARGE) !=CHARGECalcaire.end() ;

    if (t1 | t2 | t3 | t4){ aRes=1;}
    return aRes;
}

bool siglePedo::ssriche() const{
    bool aRes(0);
    bool t1 =std::find(SER_SPECRiche.begin(), SER_SPECRiche.end(), mSER_SPEC) !=SER_SPECRiche.end() ;
    if (t1){ aRes=1;}
    return aRes;
}

bool siglePedo::profond() const{
    bool aRes(0);
    bool t1 =std::find(PHASE_1Profond.begin(), PHASE_1Profond.end(), mPHASE_1) !=PHASE_1Profond.end() ;
    bool t2 =std::find(PHASE_2Profond.begin(), PHASE_2Profond.end(), mPHASE_2) !=PHASE_2Profond.end() ;
    if (t1 | t2){ aRes=1;}
    if (mPHASE_1=="" && mPHASE_2==""){ aRes=1;}
    return aRes;
}

bool siglePedo::alluvion() const{
    bool aRes(0);
    bool t1 =std::find(DEV_PROFILalluvion.begin(), DEV_PROFILalluvion.end(), mDEV_PROFIL) !=DEV_PROFILalluvion.end() ;
    if (t1){aRes=1;}
    return aRes;
}

bool siglePedo::podzol() const{
    bool aRes(0);
    if (mDEV_PROFIL=="g"){aRes=1;}
    return aRes;
}

bool siglePedo::podzolique() const{
    bool aRes(0);
      bool t1 =std::find(DEV_PROFILPodzolique.begin(),DEV_PROFILPodzolique.end(), mDEV_PROFIL) !=DEV_PROFILPodzolique.end() ;
    if (t1){aRes=1;}
    return aRes;
}
bool siglePedo::superficiel() const{
    bool aRes(0);
    if (mPHASE_1=="6" | mPHASE_2=="6"){aRes=1;}
    return aRes;
}

bool siglePedo::tourbe() const{
    bool aRes(0);
     bool t1 =std::find(MAT_TEXTtourbe.begin(),MAT_TEXTtourbe.end(), mMAT_TEXT) !=MAT_TEXTtourbe.end() ;
     bool t2 =std::find(PHASE_4tourbe.begin(),PHASE_4tourbe.end(), mPHASE_4) !=PHASE_4tourbe.end() ;
    if (t1 | t2){aRes=1;}
    return aRes;
}

bool siglePedo::limon() const{
    bool aRes(0);
     bool t1 =std::find(MAT_TEXTlimon.begin(),MAT_TEXTlimon.end(), mMAT_TEXT) !=MAT_TEXTlimon.end() ;
    if (t1){aRes=1;}
    return aRes;
}
*/
