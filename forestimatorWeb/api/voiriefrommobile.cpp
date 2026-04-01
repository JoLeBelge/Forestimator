#include "voiriefrommobile.h"

voirieFromMobile::voirieFromMobile(std::string aFileDB)
{
    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    session.setConnection(std::move(sqlite3));
    session.mapClass<observationVoirie>("observationVoirie");
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

void voirieFromMobile::handleRequest(const Http::Request &request,Http::Response &response){
    auto params = request.urlParams();
    std::string features("");
    // /api/voirieFromMobile/${feature}
    for (const auto &param : params) {
        const auto &name = param.first;
        const auto &value = param.second;
        if (name=="feature") {features=value;}
    }
    boost::replace_all(features, "'", "\"");
    GDALDataset * ds=static_cast<GDALDataset*>(GDALOpenEx(features.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
     if( ds == NULL ){
          response.out() << "NOK" ;
     } else{
    OGRLayer * lay = ds->GetLayer(0);
    OGRFeature *poFeature=lay->GetFeature(0);
    std::unique_ptr<observationVoirie> a = std::make_unique<observationVoirie>();
    a->type = poFeature->GetFieldAsString("type");
    a->categorie = poFeature->GetFieldAsString("categorie");
    a->rmq = poFeature->GetFieldAsString("rmq");
    a->nom_contact = poFeature->GetFieldAsString("nom_contact");
    a->contact = poFeature->GetFieldAsString("contact");
    a->geom = poFeature->GetGeometryRef()->exportToWkt();
    //WLocalDateTime d = WLocalDateTime::currentServerDateTime();
    //a->date = d.toString().toUTF8();
    a->date =  poFeature->GetFieldAsString("date");
    dbo::Transaction transaction(session);
    dbo::ptr<observationVoirie> aNewPolyg = session.add(std::move(a));
    response.out() << "OK" ;

    GDALClose(ds);
    }
}

