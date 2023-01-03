#include "cnsw.h"

extern std::string columnPath;
extern bool globTest;
/*
cnsw::cnsw(std::string aBDFile):dicoPedo(aBDFile)
{
}*/

cnsw::cnsw(sqlite3 *db):dicoPedo(db){
    //loadCNSW();
}

/*
void cnsw::loadCNSW(){
      GDALAllRegister();
      const char *inputPath= mShpPath.c_str();
      GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
      if( mDS == NULL )
      {
          std::cout << inputPath << " : " ;
          printf( " cnsw::loadCNSW : Open failed." );
      }

}*/

std::vector<std::string> cnsw::displayInfo(double x, double y,PEDO p){
    std::vector<std::string> aRes;
    int sol=getIndexSol(x,y);
    ptPedo ptPed=ptPedo(shared_from_this(),sol);

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
        //lay->ResetReading();
        lay->SetSpatialFilter(poGeom);

        //poGeom->MakeValid();

        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            //poFeature->GetGeometryRef()->MakeValid();
            //std::cout  << " test within" << std::endl;
            int solId=poFeature->GetFieldAsInteger("INDEX_SOL");
            if (poFeature->GetGeometryRef()->Within(poGeom)) {
                //std::cout  << " intersection des deux géométries OGRMultiPolygon" << std::endl;
                OGRMultiPolygon * pol =poFeature->GetGeometryRef()->toMultiPolygon();
                //pol->MakeValid();
                if (aRes.find(solId)==aRes.end()){ aRes.emplace(solId,pol->get_Area());} else {
                    aRes.at(solId)+=pol->get_Area();
                }

            } else {

                if (poFeature->GetGeometryRef()->Intersect(poGeom)) {
                    //std::cout  << " intersection des deux géométries " << std::endl;
                    // cette ligne foire sur le pc sentinel 2. alors que j'ai fait en sorte de placer des MakeValid pour corriger les problèmes de self intersection

                    if( poFeature->GetGeometryRef()->IsValid()){
                        OGRGeometry* pol1 = poFeature->GetGeometryRef()->Intersection(poGeom);
                        //pol1->MakeValid();
                        OGRMultiPolygon* pol = pol1->toMultiPolygon();
                        //pol->MakeValid();
                        if (aRes.find(solId)==aRes.end()){ aRes.emplace(solId,pol->get_Area());} else {
                            aRes.at(solId)+=pol->get_Area();
                        }
                    } else {
                        std::cout  << " intersection des deux géométries dans cnsw::anaSurface : problème de géométrie invalide!! " << std::endl;
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
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cnsw::loadCNSW : Open failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            OGRPoint pt(x,y);

            // OGRSpatialReference * georef =lay->GetSpatialRef();
            //lay->ResetReading();
            // cause memory leak! still reachable in loss record
            //pt.assignSpatialReference(lay->GetSpatialRef()); ne semble pas nécessaire au bon fonctionnement

            lay->SetSpatialFilter(&pt);
            //The returned feature becomes the responsibility of the caller to delete with OGRFeature::DestroyFeature().
            //while( (poFeature = lay->GetNextFeature()) != NULL )
            poFeature = lay->GetNextFeature();
            if  (poFeature!= NULL )
            {
                aRes=poFeature->GetFieldAsInteger("INDEX_SOL");
                OGRFeature::DestroyFeature(poFeature);
                //if (globTest){std::cout << "cnsw::getIndexSol --> " << aRes << std::endl;}
                //break;
            }
            //lay->SetSpatialFilter(NULL);
        }
        GDALClose(mDS);
    }
    return aRes;
}

dicoPedo::dicoPedo(sqlite3 *db):db_(db),mBDpath("jesaispas"){
    //std::cout << "dicoPedo::dicoPedo " << std::endl;
    loadInfo();
}

void dicoPedo::loadInfo(){
    sqlite3_stmt * stmt;
    std::string SQLstring="SELECT INDEX_, SIGLE_PEDO, MAT_TEXT, DRAINAGE, PHASE_1, PHASE_2, PHASE_3, PHASE_4, PHASE_5, PHASE_6, PHASE_7,CHARGE  FROM zz_Table_sigles_eclates;";
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL ){

            int index=sqlite3_column_int( stmt, 0 );
            std::string sigle=std::string( (char *)sqlite3_column_text( stmt, 1 ) );

            std::string texture("");
            if (sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
                texture=std::string( (char *)sqlite3_column_text( stmt, 2 ) );
            }
            std::string drainage("");
            if (sqlite3_column_type(stmt, 3)!=SQLITE_NULL){
               drainage=std::string( (char *)sqlite3_column_text( stmt, 3 ) );
            }

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
            if (sqlite3_column_type(stmt, 11)!=SQLITE_NULL){
                sToCharge.emplace(std::make_pair(index,std::string( (char *)sqlite3_column_text( stmt, 11 ) )));
            }
        }
    }
    sqlite3_finalize(stmt);
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

    sqlite3_finalize(stmt);
    SQLstring="SELECT CHARGE, DESCR  FROM g_charge;";
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
            std::string code=std::string( (char *)sqlite3_column_text( stmt, 0 ) );
            std::string desc=std::string( (char *)sqlite3_column_text( stmt, 1 ) );
            mCharge.emplace(std::make_pair(code,desc));
        }
    }
    sqlite3_finalize(stmt);
    SQLstring="SELECT DESCR,PHASE_1, PHASE_2, PHASE_3, PHASE_4, PHASE_5, PHASE_6, PHASE_7, DESCR_COURT  FROM i_phase;";
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
            std::string desc2=desc;
            if (sqlite3_column_type(stmt, 8)!=SQLITE_NULL){desc2=std::string( (char *)sqlite3_column_text( stmt, 8 ) );}

            std::vector<std::string> aKey{p1,p2,p3,p4,p5,p6,p7};
            mPhase.emplace(std::make_pair(aKey,desc));
            mPhaseCourt.emplace(std::make_pair(aKey,desc2));
        }

    }

    sqlite3_finalize(stmt);
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
    sqlite3_finalize(stmt);

    SQLstring="SELECT "+columnPath+",Nom FROM fichiersGIS WHERE Code='CNSW';";

    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
    if(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
            mShpPath= fs::path(std::string( (char *)sqlite3_column_text( stmt, 0 ) )+"/"+std::string( (char *)sqlite3_column_text( stmt, 1 ) ));
        }
    }
    sqlite3_finalize(stmt);
}

ptPedo::ptPedo(std::shared_ptr<cnsw> dico, int aIdSigle):mDico(dico),idSigle(aIdSigle){
    dTexture=mDico->Texture(idSigle);
    dDrainage=mDico->Drainage(idSigle);
    dProf = mDico->Profondeur(idSigle);
    dCharge = mDico->charge(idSigle);
}

ptPedo::ptPedo(std::shared_ptr<cnsw> dico, double x,double y):mDico(dico){
    idSigle=dico->getIndexSol(x,y);
    dTexture=mDico->Texture(idSigle);
    dDrainage=mDico->Drainage(idSigle);
    dProf = mDico->Profondeur(idSigle);
    dCharge = mDico->charge(idSigle);
}


std::vector<std::string> ptPedo::displayInfo(PEDO p){
    std::vector<std::string> aRes;
    std::string champ(""),desc("");
    if (idSigle!=-1){

        switch (p) {
        case PEDO::TEXTURE:{
            champ="Texture du sol";
            desc=dTexture;
            break;
        }
        case PEDO::DRAINAGE:{
            champ="Drainage du sol";
            desc=dDrainage;
            break;
        }
        case PEDO::PROFONDEUR:{
            champ="Profondeur du sol";
            desc=dProf;
            break;
        }
        case PEDO::SIGLE:{
            champ="Sigle pédologique";
            desc= mDico->sIdToSigleStr(idSigle);
            break;
        }
        case PEDO::CHARGE:{
            champ="Charge caillouteuse";
            desc=mDico->charge(idSigle);
            break;
        }
        }
    }
    aRes.push_back(champ);
    aRes.push_back(desc);
    return aRes;
}

std::string ptPedo::displayAllInfoInOverlay(){
    std::string aRes("");
    aRes+="<div class=\"alert alert-success\"> Sigle pédologique : " + displayInfo(PEDO::SIGLE).at(1) +" </div> <ul>";
    for ( int p = PEDO::DRAINAGE; p != PEDO::Last; p++ )
    {
        PEDO ped = static_cast<PEDO>(p);
        std::vector<std::string> info=displayInfo(ped);
        aRes+="<li>"+info.at(0)+": <code>"+info.at(1)+ "</code></li>";
    }
    aRes+="</ul>";
    // javascript bug si jamais l'apostrophe n'est pas escapée
    boost::replace_all(aRes,"'","\\'");
    return aRes;
}
std::string ptPedo::displayAllInfoAPI(){
    std::string aRes("");
    aRes+="Sigle pédologique;" + displayInfo(PEDO::SIGLE).at(1) +"\n";
    for ( int p = PEDO::DRAINAGE; p != PEDO::Last; p++ )
    {
        PEDO ped = static_cast<PEDO>(p);
        std::vector<std::string> info=displayInfo(ped);
        aRes+=info.at(0)+";"+info.at(1)+ "\n";
    }
    return aRes;
}

surfPedo::surfPedo(std::shared_ptr<cnsw> dico, OGRGeometry *poGeom ):mDico(dico){
    propSurf=dico->anaSurface(poGeom);

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
            if (kv.second>99.0){ dTexture+=kv.first; break;} else {
                dTexture+=kv.first+": "+mDico->roundDouble(kv.second,0)+"% ";
            }
        }
    }
    for (auto kv : VDDrainage){
        if (kv.second>1){
            if (kv.second>99.0){ dDrainage+=kv.first; break;} else {
                dDrainage+=kv.first+": "+mDico->roundDouble(kv.second,0)+"% ";}
        }
    }
    for (auto kv : VDProf){
        if (kv.second>1){
            if (kv.second>99.0){ dProf+=kv.first;break;} else {
                dProf+=kv.first+": "+mDico->roundDouble(kv.second,0)+"% ";
            }
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

std::string surfPedo::getSummaryAPI(){
    std::string aRes("");
    aRes+="Texture;"+ getSummary(PEDO::TEXTURE)+"\n";
    aRes+="Drainage;"+ getSummary(PEDO::DRAINAGE)+"\n";
    aRes+="Profondeur;"+ getSummary(PEDO::PROFONDEUR)+"\n";
    return aRes;
}

std::pair<std::string,double> surfPedo::getMajTexture(){
    std::pair<std::string,double> aRes= std::make_pair("",0);
    // 1) plusieurs sigle pedo peuvent avoir la mm Texture - on regroupe les sigles par text
    std::map<std::string,double> aVSigleTs;
    for (auto  kv : propSurf){
        std::string sText = mDico->TextureSigle(kv.first);
        if (aVSigleTs.find(sText)!=aVSigleTs.end()){ aVSigleTs.at(sText)+=kv.second;} else {
            aVSigleTs.emplace(std::make_pair(sText,kv.second));
        }
    }
    // trouver le sigle majoritaire en surface
    double aMax(0);
    for (auto & kv : aVSigleTs){
        if (kv.second>aMax){
            aMax=kv.second;
            aRes=kv;}
    }
    return aRes;
}


std::pair<std::string,double> surfPedo::getMajProf(){
    std::pair<std::string,double> aRes= std::make_pair("",0);
    // 1) plusieurs sigle pedo peuvent avoir la mm Prof - on regroupe les sigles par text
    std::map<std::string,double> aVProfs;
    for (auto  kv : propSurf){
        std::string prof = mDico->Profondeur(kv.first);
        if (aVProfs.find(prof)!=aVProfs.end()){ aVProfs.at(prof)+=kv.second;} else {
            aVProfs.emplace(std::make_pair(prof,kv.second));
        }
    }
    // trouver le sigle majoritaire en surface
    double aMax(0);
    for (auto & kv : aVProfs){
        if (kv.second>aMax){
            aMax=kv.second;
            aRes=kv;}
    }
    return aRes;
}

std::pair<std::string,double> surfPedo::getMajProfCourt(){
    std::pair<std::string,double> aRes= std::make_pair("",0);
    // 1) plusieurs sigle pedo peuvent avoir la mm Prof - on regroupe les sigles par text
    std::map<std::string,double> aVProfs;
    for (auto  kv : propSurf){
        std::string prof = mDico->ProfondeurCourt(kv.first);
        if (aVProfs.find(prof)!=aVProfs.end()){ aVProfs.at(prof)+=kv.second;} else {
            aVProfs.emplace(std::make_pair(prof,kv.second));
        }
    }
    // trouver le sigle majoritaire en surface
    double aMax(0);
    for (auto & kv : aVProfs){
        if (kv.second>aMax){
            aMax=kv.second;
            aRes=kv;}
    }
    return aRes;
}

std::pair<std::string,double> surfPedo::getMajDrainage(){
    std::pair<std::string,double> aRes= std::make_pair("",0);
    // 1) plusieurs sigle pedo peuvent avoir le MM drainage - on regroupe les sigles
    std::map<std::string,double> aVSigleDs;
    for (auto  kv : propSurf){
        std::string sD= mDico->DrainageSigle(kv.first);
        if (aVSigleDs.find(sD)!=aVSigleDs.end()){ aVSigleDs.at(sD)+=kv.second;} else {
            aVSigleDs.emplace(std::make_pair(sD,kv.second));
        }
    }
    // trouver le sigle majoritaire en surface
    double aMax(0);
    for (auto & kv : aVSigleDs){
        if (kv.second>aMax){
            aMax=kv.second;
            aRes=kv;}
    }
    return aRes;
}


std::string dicoPedo::roundDouble(double d, int precisionVal){
    std::string aRes("");
    if (precisionVal>0){aRes=std::to_string(d).substr(0, std::to_string(d).find(".") + precisionVal + 1);}
    else  {aRes=std::to_string(d+0.5).substr(0, std::to_string(d).find("."));}
    return aRes;
}
