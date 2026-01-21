#include "polygfrommobile.h"

extern bool globTest;

polygFromMobile::polygFromMobile(std::string aFileDB){
    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    session.setConnection(std::move(sqlite3));
    session.mapClass<validCompoRaster>("validCompoRaster");
    try
    {
        session.createTables();
    }
    catch (Wt::Dbo::Exception e)
    {
        //std::cout << "table creation failed" << e.code() << std::endl;
    }
    GDALAllRegister();
}

void polygFromMobile::handleRequest(const Http::Request &request,Http::Response &response){

    auto params = request.urlParams();
    std::string features("");
    // /api/polygFromMobile/${feature}
    //http://localhost:8085/api/polygFromMobile/%7B'type':'FeatureCollection','features':[%7B'type':'Feature','properties':%7B'essence':'toto','rmq':'tata','nom_contact':'titi','contact':'adresse'%7D,'geometry':%7B'type':'Point','coordinates':[146.7,-41.9]%7D%7D]%7D
    for (const auto &param : params) {
        const auto &name = param.first;
        const auto &value = param.second;
        if (name=="feature") {features=value;}
    }
    boost::replace_all(features, "'", "\"");
    if (globTest){std::cout << "feature from Mobile : "<< features << std::endl;}
    GDALDataset * ds=static_cast<GDALDataset*>(GDALOpenEx(features.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
     if( ds == NULL ){
          response.out() << "NOK" ;
     } else{
    OGRLayer * lay = ds->GetLayer(0);
    OGRFeature *poFeature=lay->GetFeature(0);
    std::unique_ptr<validCompoRaster> a = std::make_unique<validCompoRaster>();
    a->essence = poFeature->GetFieldAsString("essence");
    a->rmq = poFeature->GetFieldAsString("rmq");
    a->nom_contact = poFeature->GetFieldAsString("nom_contact");
    a->contact = poFeature->GetFieldAsString("contact");
    a->geom = poFeature->GetGeometryRef()->exportToWkt();
    dbo::Transaction transaction(session);
    dbo::ptr<validCompoRaster> aNewPolyg = session.add(std::move(a));
    response.out() << "OK" ;
    }
}
