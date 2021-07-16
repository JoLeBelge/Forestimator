#include "plaiprfw.h"

plaIPRFW::plaIPRFW(int ign, int npl, sqlite3 *db_):mIGN(ign),mNPL(npl),mGHA(0),mPctGHA(0),mAge(0),mSI(0)
{
    //std::cout << "création d'un placette iprfw" << std::endl;
    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT X,Y FROM plot WHERE IGN="+std::to_string(mIGN)+" AND NPL="+std::to_string(mNPL)+";";
    //std::cout << SQLstring << std::endl;
    sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL ){
            std::string x=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
            boost::replace_all(x, ",", ".");
            std::string y=std::string( (char *)sqlite3_column_text( stmt, 1 ) );

            boost::replace_all(y, ",", ".");

            mX=std::stod(x);
            mY=std::stod(y);
        }
    }
    sqlite3_finalize(stmt);
    SQLstring="SELECT 'ESS','GHA(m²/ha)' FROM species WHERE IGN="+std::to_string(mIGN)+" AND NPL="+std::to_string(mNPL)+";";
    boost::replace_all(SQLstring, "'", "\"");
    //std::cout << SQLstring << std::endl;
    sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
            int sp=sqlite3_column_int( stmt, 0 );
            //std::cout << " sp " << std::to_string(sp) << std::endl;
            double gha(1);
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string ghaStr=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                boost::replace_all(ghaStr, ",", ".");
                //std::cout << ghaStr << std::endl;
                gha=std::stod(ghaStr);
                mGHA+=gha;
            }
            if (sp==41){
                mPctGHA+=gha;
            }
        }
    }
    sqlite3_finalize(stmt);
    if (mGHA!=0){mPctGHA=100.0*mPctGHA/mGHA;}
    // va chercher la valeur de productivité quand celle-ci est disponible
    if (isPessiere()){

        SQLstring="SELECT date_mes,age,Siteindex FROM productivite_iprfw_2017 WHERE ign="+std::to_string(mIGN)+" AND npl="+std::to_string(mNPL)+";";

        //std::cout << SQLstring << std::endl;
        sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );
        if(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                int age=sqlite3_column_int( stmt, 1 );
               std::string date_mes=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
               //std::cout << "date mesure " << date_mes << std::endl;
               mAge=age + (2020-std::stoi(date_mes.substr(6,4)));
               mSI=sqlite3_column_double(stmt, 1 );
            }
        }
        sqlite3_finalize(stmt);
    }
    //std::cout << "création d'un placette iprfw done" << std::endl;
}

std::string plaIPRFW::summary(){
    //std::cout << "Placette " << std::to_string(mIGN) << ", "<< std::to_string(mNPL) << " position " << std::to_string(mX) << "," << std::to_string(mY) << " avec GHA " << std::to_string(mGHA) << " contient " << std::to_string(mPctGHA) << "% d'épicéa " <<std::endl;
    return std::to_string(mIGN) + ";"+ std::to_string(mNPL)+ ";"+std::to_string(mX) + ";"+ std::to_string(mY)+ ";"+std::to_string(mGHA) + ";"+ std::to_string(mPctGHA) + ";"+ std::to_string(mAge) + ";"+ std::to_string(mSI);
}
