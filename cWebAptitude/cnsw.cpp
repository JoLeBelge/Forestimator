#include "cnsw.h"

cnsw::cnsw(std::string aBDFile):dicoPedo(aBDFile)
{

}

std::vector<std::string> cnsw::displayInfo(double x, double y,PEDO p){
    std::vector<std::string> aRes;
    int sol=getIndexSol(x,y);
    if (sol!=-1){
        switch (p) {
        case PEDO::TEXTURE:{
            aRes.push_back("Texture du sol");
            std::string t(Texture(sol));
            if (t!="/"){aRes.push_back(t);}
            break;
        }
        case PEDO::DRAINAGE:{
            aRes.push_back("Drainage du sol");
            std::string d(Drainage(sol));
            if (d!="/"){aRes.push_back(d);}
            break;
        }
        case PEDO::PROFONDEUR:{
            aRes.push_back("Profondeur du sol");
            std::string p(Profondeur(sol));
            if (p!="/"){aRes.push_back(p);}
            break;
        }

    }
    }
     return aRes;
}

int cnsw::getIndexSol(double x, double y){
    int aRes(-1);
    if(!isnan(x) && !isnan(y) && !(x==0 && y==0)){
        //std::cout << "cnsw::getIndexSol " << std::endl;

        const char *inputPath= mShpPath.c_str();
        GDALAllRegister();
        GDALDataset * DS;

        DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );

        if( DS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( "Open failed." );
        } else {

            // layer
            OGRLayer * lay = DS->GetLayer(0);
            OGRFeature *poFeature;

            OGRPoint pt(x,y);
            pt.assignSpatialReference(lay->GetSpatialRef());

            lay->SetSpatialFilter(&pt);

            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                //aRes=poFeature->GetFID();
                aRes=poFeature->GetFieldAsInteger("INDEX_SOL");
                break;
                //OGRGeometry * poGeom = poFeature->GetGeometryRef();
                /* if ( pt.Within(poGeom)){
                    aRes=poFeature->GetFID();
                    break;
                }*/
            }
        }
        GDALClose( DS );
    }
    return aRes;
}

dicoPedo::dicoPedo(std::string aBDFile):mBDpath(aBDFile){
    std::cout << "dicoPedo::dicoPedo " << std::endl;


    if (openConnection()){} else {

        sqlite3_stmt * stmt;
        std::string SQLstring="SELECT INDEX_, SIGLE_PEDO, MAT_TEXT, DRAINAGE, PHASE_1  FROM zz_Table_sigles_eclates;";
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL  && sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
                int index=sqlite3_column_int( stmt, 0 );
                std::string sigle=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string texture=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                std::string drainage=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
                sigleIdToSigleStr.emplace(std::make_pair(index,sigle));
                sToTexture.emplace(std::make_pair(index,texture));
                sToDrainage.emplace(std::make_pair(index,drainage));
                if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL){
                    sToPhase.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 4 ) )));
                }
            }
        }

        sqlite3_reset(stmt);
        SQLstring="SELECT MAT_TEXT, DESCR  FROM b_texture;";
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string code=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string desc=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                mTexture.emplace(std::make_pair(code,desc));
            }
        }
        sqlite3_reset(stmt);
        SQLstring="SELECT PHASE_1, DESCR  FROM i_phase;";
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                std::string code=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string desc=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                mPhase.emplace(std::make_pair(code,desc));
            }
        }

        sqlite3_reset(stmt);
        SQLstring="SELECT TEXTURE,DRAINAGE, DESCR  FROM c_drainage;";
        sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        while(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){

                std::string ComplexeTexture=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
                std::string drainage=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
                std::string desc=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
                std::vector<std::string> aVTexture;
                boost::split(aVTexture,ComplexeTexture,boost::is_any_of(", "),boost::token_compress_on);
                for (std::string texture: aVTexture){
                    //std::cout << " texture " << texture << " trouvé dans complexe de T." << std::endl;
                    mDrainage.emplace(std::make_pair(std::make_pair(texture,drainage),desc));
                }
            }
        }

        // changer la requete en fonction de la machine sur laquelle est installé l'appli
        char userName[20];
        getlogin_r(userName,sizeof(userName));
        std::string s(userName);
        if (s=="lisein"){
            SQLstring="SELECT Dir2,Nom FROM fichiersGIS WHERE Code='CNSW';";
        } else {
            SQLstring="SELECT Dir,Nom FROM fichiersGIS WHERE Code='CNSW';";
        }
         sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
        if(sqlite3_step(stmt) == SQLITE_ROW)
        {
            if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                mShpPath= fs::path(std::string( (char *)sqlite3_column_text( stmt, 0 ) )+"/"+std::string( (char *)sqlite3_column_text( stmt, 1 ) ));
            }
        }

        sqlite3_finalize(stmt);


    }
    closeConnection();
}

int dicoPedo::openConnection(){
    int rc;

    std::cout << "chargement des dictionnaires lié à la couche pédo ..." << std::endl;

    rc = sqlite3_open_v2(mBDpath.c_str(), &db_,SQLITE_OPEN_READONLY,NULL);
    // The 31 result codes are defined in sqlite3.h
    //SQLITE_ERROR (1)
    if( rc!=0) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
        std::cout << " mBDpath " << mBDpath << std::endl;
        std::cout << "result code : " << rc << std::endl;
    }

    return rc;
}

void dicoPedo::closeConnection(){

    int rc = sqlite3_close_v2(db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(db_));
    }
}


