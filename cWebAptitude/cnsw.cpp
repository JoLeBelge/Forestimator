#include "cnsw.h"

cnsw::cnsw(std::string aBDFile):dicoPedo(aBDFile)
{

}

cnsw::cnsw(sqlite3 * db):dicoPedo(db){

}

std::vector<std::string> cnsw::displayInfo(double x, double y,PEDO p){
    std::vector<std::string> aRes;
    int sol=getIndexSol(x,y);
    ptPedo ptPed=ptPedo(this,sol);

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

std::map<int,double> cnsw::anaSurface(OGRGeometry *poGeom){
    std::map<int,double> aRes;
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

        lay->SetSpatialFilter(poGeom);

        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            int solId=poFeature->GetFieldAsInteger("INDEX_SOL");
            if (poFeature->GetGeometryRef()->Within(poGeom)) {
                std::cout  << " intersection des deux géométries " << std::endl;

                OGRMultiPolygon * pol =poFeature->GetGeometryRef()->toMultiPolygon();
                if (aRes.find(solId)==aRes.end()){ aRes.emplace(solId,pol->get_Area());} else {
                    aRes.at(solId)+=pol->get_Area();
                }

            } else {

                if (poFeature->GetGeometryRef()->Intersect(poGeom)) {
                    std::cout  << " intersection des deux géométries " << std::endl;
                    OGRMultiPolygon* pol = poFeature->GetGeometryRef()->Intersection(poGeom)->toMultiPolygon();
                    if (aRes.find(solId)==aRes.end()){ aRes.emplace(solId,pol->get_Area());} else {
                        aRes.at(solId)+=pol->get_Area();
                    }
                }
            }

        }
    }
    GDALClose( DS );

    double surfTot=poGeom->toMultiPolygon()->get_Area();
    //double pctTot(0);
    for (auto & kv : aRes){
        kv.second=(100.0*kv.second)/surfTot;
        //  pctTot+=kv.second;
    }
    //std::cout << " somme des pourcentage de surface ; " << pctTot << std::endl;

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

dicoPedo::dicoPedo(sqlite3 * db):db_(db),mBDpath("jesaispas"){
    std::cout << "dicoPedo::dicoPedo " << std::endl;

    loadInfo();
}

void dicoPedo::loadInfo(){
    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT INDEX_, SIGLE_PEDO, MAT_TEXT, DRAINAGE, PHASE_1, PHASE_2, PHASE_3, PHASE_4, PHASE_5, PHASE_6, PHASE_7  FROM zz_Table_sigles_eclates;";
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
                sToPhase1.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 4 ) )));
            }
            if (sqlite3_column_type(stmt, 5)!=SQLITE_NULL){
                sToPhase2.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 5 ) )));
            }
            if (sqlite3_column_type(stmt, 6)!=SQLITE_NULL){
                sToPhase3.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 6 ) )));
            }
            if (sqlite3_column_type(stmt, 7)!=SQLITE_NULL){
                sToPhase4.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 7 ) )));
            }
            if (sqlite3_column_type(stmt, 8)!=SQLITE_NULL){
                sToPhase5.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 8 ) )));
            }
            if (sqlite3_column_type(stmt, 9)!=SQLITE_NULL){
                sToPhase6.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 9 ) )));
            }
            if (sqlite3_column_type(stmt, 10)!=SQLITE_NULL){
                sToPhase7.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 10 ) )));
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
    SQLstring="SELECT  DESCR,PHASE_1, PHASE_2, PHASE_3, PHASE_4, PHASE_5, PHASE_6, PHASE_7  FROM i_phase;";
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL){
            std::string desc=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
            std::string p1(""),p2(""),p3(""),p4(""),p5(""),p6(""),p7("");
            if (sqlite3_column_type(stmt, 1)!=SQLITE_NULL){p1=std::string( (char *)sqlite3_column_text( stmt, 1 ) );}
            if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){p2=std::string( (char *)sqlite3_column_text( stmt, 2 ) );}
            if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL){p3=std::string( (char *)sqlite3_column_text( stmt, 3 ) );}
            if (sqlite3_column_type(stmt, 4)!=SQLITE_NULL){p4=std::string( (char *)sqlite3_column_text( stmt, 4 ) );}
            if (sqlite3_column_type(stmt, 5)!=SQLITE_NULL){p5=std::string( (char *)sqlite3_column_text( stmt, 5 ) );}
            if (sqlite3_column_type(stmt, 6)!=SQLITE_NULL){p6=std::string( (char *)sqlite3_column_text( stmt, 6 ) );}
            if (sqlite3_column_type(stmt, 7)!=SQLITE_NULL){p7=std::string( (char *)sqlite3_column_text( stmt, 7 ) );}
            std::vector<std::string> aKey{p1,p2,p3,p4,p5,p6,p7};
            mPhase.emplace(std::make_pair(aKey,desc));
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




dicoPedo::dicoPedo(std::string aBDFile):mBDpath(aBDFile),db_(NULL){
    std::cout << "dicoPedo::dicoPedo " << mBDpath << std::endl;

    if (openConnection()){} else {

        loadInfo();
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

    std::cout << "close connection" << std::endl;
    int rc = sqlite3_close_v2(db_);
    if( rc ) {
        fprintf(stderr, "Can't close database: %s\n\n\n", sqlite3_errmsg(db_));
    }
}

ptPedo::ptPedo(dicoPedo * dico,int aIdSigle):mDico(dico),idSigle(aIdSigle){

    mDico= dico;
    dTexture=mDico->Texture(idSigle);
    dDrainage=mDico->Drainage(idSigle);
    dProf = mDico->Profondeur(idSigle);
}

ptPedo::ptPedo(cnsw * dico, double x,double y):mDico(dico){
    idSigle=dico->getIndexSol(x,y);
    dTexture=mDico->Texture(idSigle);
    dDrainage=mDico->Drainage(idSigle);
    dProf = mDico->Profondeur(idSigle);
}


std::vector<std::string> ptPedo::displayInfo(PEDO p){
    std::vector<std::string> aRes;

    if (idSigle!=-1){
        switch (p) {
        case PEDO::TEXTURE:{
            aRes.push_back("Texture du sol");
            if (dTexture!="/"){aRes.push_back(dTexture);}
            break;
        }
        case PEDO::DRAINAGE:{
            aRes.push_back("Drainage du sol");
            if (dDrainage!="/"){aRes.push_back(dDrainage);}
            break;
        }
        case PEDO::PROFONDEUR:{
            aRes.push_back("Profondeur du sol");
            if (dProf!="/"){aRes.push_back(dProf);}
            break;
        }

        }
    }
    return aRes;
}


surfPedo::surfPedo(cnsw * dico, OGRGeometry *poGeom ):mDico(dico){
    propSurf=dico->anaSurface(poGeom);

    // synthèse des statistiques dans 3 chaines de charactères
    std::map<std::string, double> VDDrainage;
    std::map<std::string, double> VDText;
    std::map<std::string, double> VDProf;

    for (auto & kv : propSurf){
        int idSigle = kv.first;
        double prop=kv.second;
        std::string t=mDico->Texture(idSigle);
        if (t!="/"){
            if (VDText.find(t)==VDText.end()){ VDText.emplace(t,prop);} else {
                VDText.at(t)+=prop;
            }
        }
        std::string d=mDico->Drainage(idSigle);
        if (d!="/"){
            if (VDDrainage.find(d)==VDDrainage.end()){ VDDrainage.emplace(d,prop);} else {
                VDDrainage.at(d)+=prop;
            }
        }
        std::string p=mDico->Profondeur(idSigle);
        if (p!="/"){
            if (VDProf.find(p)==VDProf.end()){ VDProf.emplace(p,prop);} else {
                VDProf.at(p)+=prop;
            }
        }
    }

    for (auto kv : VDText){
        if (kv.second>1){
            if (kv.second>99.0){ dTexture+=kv.first;}
            dTexture+=kv.first+": "+roundDouble(kv.second,0)+"% ";
        }
    }
    for (auto kv : VDDrainage){
        if (kv.second>1){
            if (kv.second>99.0){ dDrainage+=kv.first;}
            dDrainage+=kv.first+": "+roundDouble(kv.second,0)+"% ";
        }
    }
    for (auto kv : VDProf){
        if (kv.second>1){
            if (kv.second>99.0){ dProf+=kv.first;}
            dProf+=kv.first+": "+roundDouble(kv.second,0)+"% ";
        }
    }
}

std::string surfPedo::getSummary(PEDO p){
    std::string aRes("");
        switch (p) {
        case PEDO::TEXTURE:{
            aRes= dTexture;
            break;
        }
        case PEDO::DRAINAGE:{
              aRes= dDrainage;
            break;
        }
        case PEDO::PROFONDEUR:{
              aRes= dProf;
            break;
        }

    }
    return aRes;
}


