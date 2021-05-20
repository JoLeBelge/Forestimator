#include "cadastre.h"

cadastre::cadastre(sqlite3 *db)
{
    db_=db;
    // les chemins d'accès vers les shp
    GDALAllRegister();
    loadInfo();

}

void cadastre::loadInfo(){
    sqlite3_stmt * stmt;
    // changer la requete en fonction de la machine sur laquelle est installé l'appli
    char userName[20];
    getlogin_r(userName,sizeof(userName));
    std::string s(userName),SQLstring;

    if (s=="lisein"){
        SQLstring="SELECT Dir2,Nom,Code FROM fichiersGIS WHERE Categorie='Cadastre';";
    } else {
        SQLstring="SELECT Dir,Nom,Code FROM fichiersGIS WHERE Categorie='Cadastre';";
    }
    sqlite3_prepare_v2( db_, SQLstring.c_str(), -1, &stmt, NULL );
    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL  && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
            std::string code=std::string((char *)sqlite3_column_text( stmt, 2 ));
            if (code=="Commune"){
                mShpCommunePath=fs::path(std::string( (char *)sqlite3_column_text( stmt, 0 ) )+"/"+std::string( (char *)sqlite3_column_text( stmt, 1 ) ));
            }else if (code=="Division"){
                mShpDivisionPath=fs::path(std::string( (char *)sqlite3_column_text( stmt, 0 ) )+"/"+std::string( (char *)sqlite3_column_text( stmt, 1 ) ));
            }else if (code=="PaCa"){
                mShpParcellePath=fs::path(std::string( (char *)sqlite3_column_text( stmt, 0 ) )+"/"+std::string( (char *)sqlite3_column_text( stmt, 1 ) ));
            }
        }
    }
    sqlite3_finalize(stmt);

    // lecture des communes
    const char *inputPath= mShpCommunePath.c_str();
    if (boost::filesystem::exists(inputPath)){
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cadastre : Open communes failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                // on garde que la région wallonne
                //std::cout << poFeature->GetFieldAsString("NameFRE") << " , " << poFeature->GetFieldAsInteger("AdReKey") << std::endl;
                if (poFeature->GetFieldAsInteger("AdReKey")==3000){
                    mVCom.emplace(std::make_pair(poFeature->GetFieldAsInteger("AdMuKey"),poFeature->GetFieldAsString("NameFRE")));
                }
            }
        }
        GDALClose(mDS);
    } else {
        std::cout << inputPath << " n'existe pas " ;
    }

    // lecture des divisions
    inputPath= mShpDivisionPath.c_str();
    if (boost::filesystem::exists(inputPath)){
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cadastre : Open division failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                // garder que ceux qui sont dans une commune de RW
                if (mVCom.find(poFeature->GetFieldAsInteger("AdMuKey"))!=mVCom.end()){
                    mVDiv.emplace(std::make_pair(poFeature->GetFieldAsInteger("CaDiKey"), std::make_tuple(poFeature->GetFieldAsInteger("AdMuKey"),poFeature->GetFieldAsString("NameFRE"))));
                }
            }
        }
        GDALClose(mDS);
    } else {
        std::cout << inputPath << " n'existe pas " ;
    }

    // lecture des parcelles cadastrales - il y en a 4 000 000 donc je ne vais pas tout lire, sinon c'est un peu lent.
    inputPath= mShpParcellePath.c_str();
    if (boost::filesystem::exists(inputPath)){
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cadastre : Open parcelles cadastrales failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                // création d'une parcelle cadastrale et ajout au vecteur
                std::unique_ptr<capa> pt=std::make_unique<capa>(poFeature->GetFieldAsString("CaSeKey"),poFeature->GetFieldAsString("CaPaKey"),&mVCom,&mVDiv);

                if (mVCaPa.find(pt->divCode)==mVCaPa.end()){
                    mVCaPa.emplace(std::make_pair(pt->divCode,std::vector<std::unique_ptr<capa>>()));
                }
                mVCaPa.at(pt->divCode).push_back(std::move(pt));
            }
        }
        GDALClose(mDS);
    } else {
        std::cout << inputPath << " n'existe pas " ;
    }
    /*
    std::cout << " j'ai lu " << mVCom.size() << " communes du cadastres (belgique)" << std::endl;
    std::cout << " j'ai lu " << mVDiv.size() << " divisions du cadastres (belgique)" << std::endl;
    std::cout << " j'ai rangé les parcelles cadastrales de Wallonie dans " << mVCaPa.size() << " vecteurs " << std::endl;
    std::vector<std::string> test =getSectionForDiv(61007);
    for (auto & s : test){std::cout << " section " << s << std::endl;}
    std::vector<capa *> t=getCaPaPtrVector(std::get<0>(mVDiv.at(61003)),"A");
    std::cout << " nombre de pointers capa" << t.size() << std::endl;
    */

}


capa::capa(std::string aCaSecKey, std::string aCaPaKey, std::map<int,std::string> * aVCom, std::map<int,std::tuple<int,std::string>> * aVDiv):
    CaSecKey(aCaSecKey),CaPaKey(aCaPaKey),comINS(0){
    // détermine le code division et la section depuis CaSecKey
    divCode=std::stoi(CaSecKey.substr(0,CaSecKey.size()-1));
    section=CaSecKey.substr(CaSecKey.size()-1,CaSecKey.size());
    if( aVDiv->find(divCode)!=aVDiv->end()){
        comINS=std::get<0>(aVDiv->at(divCode));
    }

    //std::cout << "Parcelle Cadastrale " << CaPaKey << ", section " << section << " de la division " << divCode << " , commune " << comINS<< std::endl;
}

std::vector<std::string> cadastre::getSectionForDiv(int aDivCode){
    std::vector<std::string> aRes;
    if (mVCaPa.find(aDivCode)!=mVCaPa.end()){
        for (std::unique_ptr<capa> & parcelle : mVCaPa.at(aDivCode)){
            if (std::find(aRes.begin(), aRes.end(), parcelle->section) == aRes.end()){aRes.push_back(parcelle->section);}
        }
    }
    std::sort(aRes.begin(), aRes.end());
    return aRes;
}
std::vector<capa *> cadastre::getCaPaPtrVector(int aDivCode,std::string aSection){
    std::vector<capa *> aRes;
    if (mVCaPa.find(aDivCode)!=mVCaPa.end()){
        for (std::unique_ptr<capa> & parcelle : mVCaPa.at(aDivCode)){
            if (parcelle->section==aSection){aRes.push_back(parcelle.get());}
        }
    }
    return aRes;
}

std::string cadastre::createPolygonDiv(int aDivCode){
    std::string aRes;
    const char *inputPath= mShpDivisionPath.c_str();
    if (boost::filesystem::exists(inputPath)){
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cadastre : Open division failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                if (poFeature->GetFieldAsInteger("CaDiKey")==aDivCode){
                    // j'exporte ce polygone au format json
                    aRes=poFeature->GetGeometryRef()->exportToJson();
                    break;
                }
            }
        }
        GDALClose(mDS);
    } else {
        std::cout << inputPath << " n'existe pas " ;
    }
    return aRes;
}
std::string cadastre::createPolygonCommune(int aINScode){
    std::string aRes;
    const char *inputPath= mShpCommunePath.c_str();
    if (boost::filesystem::exists(inputPath)){
        GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
        if( mDS == NULL )
        {
            std::cout << inputPath << " : " ;
            printf( " cadastre : Open commune failed." );
        } else{
            // layer
            OGRLayer * lay = mDS->GetLayer(0);
            OGRFeature *poFeature;
            while( (poFeature = lay->GetNextFeature()) != NULL )
            {
                if (poFeature->GetFieldAsInteger("AdMuKey")==aINScode){
                    // j'exporte ce polygone au format json
                    aRes=featureToGeoJSON(poFeature);
                    break;
                }
            }
        }
        GDALClose(mDS);
    } else {
        std::cout << inputPath << " n'existe pas " ;
    }
    return aRes;
}


std::string featureToGeoJSON(OGRFeature *f)
{
    std::string aRes;

    aRes+="{\"type\":\"FeatureCollection\", crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:EPSG::31370\" } },\"features\":[";
    //Geometry
    aRes+="{\"type\":\"Feature\",\"geometry\":" + std::string(f->GetGeometryRef()->exportToJson()) + ",";

    //Properties
    int count = f->GetFieldCount();
    if (count!=0)
    {
        aRes+="\"properties\":{";
        for (int i = 0; i < count; i++)
        {
            OGRFieldType type = f->GetFieldDefnRef(i)->GetType();
            std::string key = f->GetFieldDefnRef(i)->GetNameRef();

            if (type == OFTInteger)
            {
                int field = f->GetFieldAsInteger(i);
                aRes+="\"" + key + "\":" + std::to_string(field) + ",";
            }
            else if (type == OFTReal)
            {
                double field = f->GetFieldAsDouble(i);
                aRes+="\"" + key + "\":" + std::to_string(field) + ",";
            }
            else
            {
                std::string field = f->GetFieldAsString(i);
                aRes+="\"" + key + "\":\"" + field + "\",";
            }

        }

        aRes+="},";
    }
    //FID
    long id = f->GetFID();
    aRes+="\"id\":" + std::to_string(id) + "},";

    aRes+="]}";

    return aRes;
}
//std::string createPolygonCaPa(int aDivCode,std::string aCaPaKey);
