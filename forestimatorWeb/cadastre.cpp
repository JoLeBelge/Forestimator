#include "cadastre.h"
#include <boost/filesystem.hpp>
extern bool globTest;
extern std::string columnPath;

inline bool exists(const std::string &name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

cadastre::cadastre(sqlite3 *db) : mShpCommunePath(""), mShpDivisionPath(""), mShpParcellePath(""), mTmpDir(""), mDirBDCadastre("")
{
    if (globTest)
    {
        std::cout << " création classe cadastre.." << std::endl;
    }
    db_ = db;
    // les chemins d'accès vers les shp
    GDALAllRegister();
    loadInfo();
}

void cadastre::loadInfo()
{
    sqlite3_stmt *stmt;

    std::cout << "load info cadastre" << std::endl;
    const char *query = "SELECT ?,Nom,Code FROM fichiersGIS WHERE Categorie='Cadastre' OR Code='TMPDIR'";

    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, columnPath.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL  && sqlite3_column_type(stmt, 2)!=SQLITE_NULL){
            if (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
            {
                std::string code = std::string((char *)sqlite3_column_text(stmt, 2));
                if (code == "Commune")
                {
                    mShpCommunePath = fs::path(std::string((char *)sqlite3_column_text(stmt, 0)) + "/" + std::string((char *)sqlite3_column_text(stmt, 1)));
                }
                else if (code == "Division")
                {
                    mShpDivisionPath = fs::path(std::string((char *)sqlite3_column_text(stmt, 0)) + "/" + std::string((char *)sqlite3_column_text(stmt, 1)));
                }
                else if (code == "PaCa")
                {
                    mShpParcellePath = fs::path(std::string((char *)sqlite3_column_text(stmt, 0)) + "/" + std::string((char *)sqlite3_column_text(stmt, 1)));
                }
                else if (code == "TMPDIR")
                {
                    mTmpDir = std::string((char *)sqlite3_column_text(stmt, 0));
                }
                else if (code == "CadastreBD")
                {
                    mDirBDCadastre = std::string((char *)sqlite3_column_text(stmt, 0)) + "/" + std::string((char *)sqlite3_column_text(stmt, 1));
                }
            }
            sqlite3_finalize(stmt);
        }

        // lecture des communes
        const char *inputPath = mShpCommunePath.c_str();
        if (boost::filesystem::exists(inputPath))
        {
            if (globTest)
            {
                std::cout << "open gdaldataset" << std::endl;
            }
            GDALDataset *mDS;
            mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if (mDS == NULL)
            {
                std::cout << inputPath << " : ";
                printf(" cadastre : Open communes failed.");
            }
            else
            {
                // layer
                OGRLayer *lay = mDS->GetLayer(0);
                OGRFeature *poFeature;
                while ((poFeature = lay->GetNextFeature()) != NULL)
                {
                    // on garde que la région wallonne
                    // std::cout << poFeature->GetFieldAsString("NameFRE") << " , " << poFeature->GetFieldAsInteger("AdReKey") << std::endl;
                    if (poFeature->GetFieldAsInteger("AdReKey") == 3000)
                    {
                        mVCom.emplace(std::make_pair(poFeature->GetFieldAsInteger("AdMuKey"), poFeature->GetFieldAsString("NameFRE")));
                    }
                }
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << inputPath << " n'existe pas ";
        }

        // lecture des divisions
        inputPath = mShpDivisionPath.c_str();
        if (boost::filesystem::exists(inputPath))
        {
            GDALDataset *mDS;
            mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if (mDS == NULL)
            {
                std::cout << inputPath << " : ";
                printf(" cadastre : Open division failed.");
            }
            else
            {
                // layer
                OGRLayer *lay = mDS->GetLayer(0);
                OGRFeature *poFeature;
                while ((poFeature = lay->GetNextFeature()) != NULL)
                {
                    // garder que ceux qui sont dans une commune de RW
                    if (mVCom.find(poFeature->GetFieldAsInteger("AdMuKey")) != mVCom.end())
                    {
                        mVDiv.emplace(std::make_pair(poFeature->GetFieldAsInteger("CaDiKey"), std::make_tuple(poFeature->GetFieldAsInteger("AdMuKey"), poFeature->GetFieldAsString("NameFRE"))));
                    }
                }
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << inputPath << " n'existe pas ";
        }

        // lecture des parcelles cadastrales - il y en a 4 000 000 donc je ne vais pas tout lire, sinon c'est un peu lent.
        // inputPath= mShpParcellePath.c_str();
        // si j'ouvre uniquement le dbf, est-ce que je ne gagne pas un peu de temps? apparemment pas...
        /*  std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3("cadastre.db")};
        dbo::Session session;
        session.setConnection(std::move(sqlite3));
        session.mapClass<capa>("capa");
        //session.createTables();
        dbo::Transaction transaction{session}; // une seule transaction pour l'ajout de tout
        */
        // ça c'est fait uniquement une fois pour créer la bd sqlite
        if (0)
        {
            std::string tmp = mShpParcellePath.c_str();
            tmp = tmp.substr(0, mShpParcellePath.size() - 3) + "dbf";
            inputPath = tmp.c_str();
            if (boost::filesystem::exists(inputPath))
            {
                GDALDataset *mDS;
                mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
                if (mDS == NULL)
                {
                    std::cout << inputPath << " : ";
                    printf(" cadastre : Open parcelles cadastrales failed.");
                }
                else
                {
                    // layer
                    OGRLayer *lay = mDS->GetLayer(0);
                    OGRFeature *poFeature;
                    int i(0);
                    while ((poFeature = lay->GetNextFeature()) != NULL)
                    {
                        // création d'une parcelle cadastrale et ajout au vecteur
                        std::unique_ptr<capa> pt = std::make_unique<capa>(poFeature->GetFieldAsString("CaSeKey"), poFeature->GetFieldAsString("CaPaKey"), poFeature->GetFID(), &mVDiv);

                        /* if (mVCaPa.find(pt->divCode)==mVCaPa.end()){
                        mVCaPa.emplace(std::make_pair(pt->divCode,std::vector<std::unique_ptr<capa>>()));
                    }

                    mVCaPa.at(pt->divCode).push_back(std::move(pt));
                    */
                        // ajout de la parcelle à la bd sqlite via ORM de wt

                        // dbo::ptr<capa> capaPtr = session.add(std::move(pt));

                        i++;
                        if (i % 10000 == 0)
                        {
                            std::cout << " i " << i << std::endl;
                        }
                    }
                }
                GDALClose(mDS);
            }
            else
            {
                std::cout << inputPath << " n'existe pas ";
            }
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

    capa::capa(std::string aCaSecKey, std::string aCaPaKey, int aPID, std::map<int, std::tuple<int, std::string>> *aVDiv) : CaSecKey(aCaSecKey), CaPaKey(aCaPaKey), comINS(0), mPID(aPID)
    {
        // détermine le code division et la section depuis CaSecKey
        divCode = std::stoi(CaSecKey.substr(0, CaSecKey.size() - 1));
        section = CaSecKey.substr(CaSecKey.size() - 1, CaSecKey.size());
        if (aVDiv->find(divCode) != aVDiv->end())
        {
            comINS = std::get<0>(aVDiv->at(divCode));
        }
    }

    std::vector<std::string> cadastre::getSectionForDiv(int aDivCode, Wt::Dbo::Session *session)
    {
        std::vector<std::string> aRes;

        /* a modifier, maintenant que je vais lire l'objet mappé capa.
        if (mVCaPa.find(aDivCode)!=mVCaPa.end()){
            for (std::unique_ptr<capa> & parcelle : mVCaPa.at(aDivCode)){
                if (std::find(aRes.begin(), aRes.end(), parcelle->section) == aRes.end()){aRes.push_back(parcelle->section);}
            }
        }*/
        typedef dbo::collection<dbo::ptr<capa>> collectionCapa;
        dbo::Transaction transaction{*session};
        collectionCapa Capas = session->find<capa>().where("divCode = ?").bind(aDivCode);

        for (dbo::ptr<capa> &c : Capas)
        {
            if (std::find(aRes.begin(), aRes.end(), c->section) == aRes.end())
            {
                aRes.push_back(c->section);
            }
        }

        std::sort(aRes.begin(), aRes.end());
        return aRes;
    }

    std::vector<dbo::ptr<capa>> cadastre::getCaPaPtrVector(int aDivCode, std::string aSection, Wt::Dbo::Session *session)
    {

        std::vector<dbo::ptr<capa>> aRes;
        typedef dbo::collection<dbo::ptr<capa>> collectionCapa;
        dbo::Transaction transaction{*session};
        // collectionCapa Capas = session->find<capa>().where("divCode = ?").bind(aDivCode).orderBy("CaPaKey");
        collectionCapa Capas = session->find<capa>().where("CaSecKey = ?").bind(std::to_string(aDivCode) + aSection).orderBy("CaPaKey");

        for (dbo::ptr<capa> &c : Capas)
        {
            aRes.push_back(c);
        }

        return aRes;
    }

    dbo::ptr<capa> cadastre::getCaPaPtr(std::string aCaPaKey, dbo::Session * session)
    {
        dbo::Transaction transaction{*session};
        dbo::ptr<capa> aRes = session->find<capa>().where("CaPaKey = ?").bind(aCaPaKey);
        boost::replace_all(aCaPaKey, "/", "\/"); // ben oui sinon ça marche pas le sql select de dbo wt si on échappe pas le slash...
        // aRes = session->find<capa>().where("CaPaKey = ?").bind(aCaPaKey);
        // collectionCapa Capas = session->find<capa>().where("CaPaKey = ?").bind(aCaPaKey);
        // std::cout << Capas.size() <<" est le nombre de capa retourné par la méthode" << std::endl;
        return aRes;
    }

    std::string cadastre::createPolygonDiv(int aDivCode)
    {
        std::string aRes;
        const char *inputPath = mShpDivisionPath.c_str();
        if (boost::filesystem::exists(inputPath))
        {
            GDALDataset *mDS;
            mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if (mDS == NULL)
            {
                std::cout << inputPath << " : ";
                printf(" cadastre : Open division failed.");
            }
            else
            {
                // layer
                OGRLayer *lay = mDS->GetLayer(0);
                OGRFeature *poFeature;
                while ((poFeature = lay->GetNextFeature()) != NULL)
                {
                    if (poFeature->GetFieldAsInteger("CaDiKey") == aDivCode)
                    {
                        // j'exporte ce polygone au format json
                        aRes = saveFeatAsGEOJSON(poFeature);
                        // break;
                        GDALClose(mDS);
                        return aRes;
                        // bug too many files open
                    }
                }
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << inputPath << " n'existe pas ";
        }
        return aRes;
    }
    std::string cadastre::createPolygonCommune(int aINScode)
    {
        std::string aRes;
        const char *inputPath = mShpCommunePath.c_str();
        if (boost::filesystem::exists(inputPath))
        {
            GDALDataset *mDS;
            mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if (mDS == NULL)
            {
                std::cout << inputPath << " : ";
                printf(" cadastre : Open commune failed.");
            }
            else
            {
                // layer
                OGRLayer *lay = mDS->GetLayer(0);
                OGRFeature *poFeature;
                while ((poFeature = lay->GetNextFeature()) != NULL)
                {
                    if (poFeature->GetFieldAsInteger("AdMuKey") == aINScode)
                    {
                        // j'exporte ce polygone au format json, je le sauve dans un fichier, me retourne le nom du fichier
                        aRes = saveFeatAsGEOJSON(poFeature);
                        // break;
                        GDALClose(mDS);
                        return aRes;
                        // bug too many files open
                    }
                }
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << inputPath << " n'existe pas ";
        }
        return aRes;
    }
    /*
    std::string cadastre::createPolygonPaCa(std::string aCaPaKey, int divCode){
        std::string aRes;
        const char *inputPath= mShpParcellePath.c_str();
        if (boost::filesystem::exists(inputPath)){
            GDALDataset * mDS; mDS= GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if( mDS == NULL )
            {
                std::cout << inputPath << " : " ;
                printf( " cadastre : Open parcelles failed." );
            } else{
                // layer
                OGRLayer * lay = mDS->GetLayer(0);
               // OGRFeature *poFeature=lay->GetFeature();
                // j'exporte ce polygone au format json, je le sauve dans un fichier, me retourne le nom du fichier
                aRes=saveFeatAsGEOJSON(poFeature);


            }
            GDALClose(mDS);
        } else {
            std::cout << inputPath << " n'existe pas " ;
        }
        return aRes;
    }*/

    std::string cadastre::createPolygonPaCa(int aFID)
    {
        std::string aRes;
        const char *inputPath = mShpParcellePath.c_str();
        if (boost::filesystem::exists(inputPath))
        {
            GDALDataset *mDS;
            mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
            if (mDS == NULL)
            {
                std::cout << inputPath << " : ";
                printf(" cadastre : Open parcelles failed.");
            }
            else
            {
                // layer
                OGRLayer *lay = mDS->GetLayer(0);
                OGRFeature *poFeature = lay->GetFeature(aFID);
                // j'exporte ce polygone au format json, je le sauve dans un fichier, me retourne le nom du fichier
                aRes = saveFeatAsGEOJSON(poFeature);
            }
            GDALClose(mDS);
        }
        else
        {
            std::cout << inputPath << " n'existe pas ";
        }
        return aRes;
    }

    std::string featureToGeoJSON(OGRFeature * f)
    {
        std::string aRes;
        aRes += "{\"type\":\"FeatureCollection\",\n \"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:EPSG::31370\" } },\n \"features\":[\n";
        // Geometry
        aRes += "{\"type\":\"Feature\",\"geometry\":" + std::string(f->GetGeometryRef()->exportToJson()) + ",\n";
        // Properties
        int count = f->GetFieldCount();
        if (count != 0)
        {
            aRes += "\"properties\":{";
            for (int i = 0; i < count; i++)
            {
                OGRFieldType type = f->GetFieldDefnRef(i)->GetType();
                std::string key = f->GetFieldDefnRef(i)->GetNameRef();

                if (type == OFTInteger)
                {
                    int field = f->GetFieldAsInteger(i);
                    aRes += "\"" + key + "\":" + std::to_string(field);
                }
                else if (type == OFTReal)
                {
                    double field = f->GetFieldAsDouble(i);
                    aRes += "\"" + key + "\":" + std::to_string(field);
                }
                else
                {
                    std::string field = f->GetFieldAsString(i);
                    aRes += "\"" + key + "\":\"" + field + "\"";
                }
                if (i + 1 != count)
                {
                    aRes += +",";
                }
            }

            aRes += "}\n";
        }
        aRes += "}\n";
        aRes += "]}\n";
        return aRes;
    }

    std::string cadastre::saveFeatAsGEOJSON(OGRFeature * f)
    {
        // Use a safe unique temporary filename
        boost::filesystem::path tmpPath = boost::filesystem::path(mTmpDir) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.geojson");
        std::string aOut = tmpPath.string();
        std::string pol = featureToGeoJSON(f);
        std::ofstream ofs(aOut, std::ofstream::out);
        ofs << pol;
        ofs.close();
        return aOut;
    }

    void cadastre::getCaPa4pt(double x, double y, ptCadastre *aPt)
    {
        if (!isnan(x) && !isnan(y) && !(x == 0 && y == 0))
        {
            const char *inputPath = mShpParcellePath.c_str();
            if (boost::filesystem::exists(inputPath))
            {
                GDALDataset *mDS;
                mDS = GDALDataset::Open(inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY);
                if (mDS == NULL)
                {
                    std::cout << inputPath << " : ";
                    printf(" cadastre : Open parcelles failed.");
                }
                else
                {
                    // std::cout << "getCaPaKey pour une position donnée" << std::endl;
                    OGRLayer *lay = mDS->GetLayer(0);
                    OGRFeature *poFeature;
                    OGRPoint pt(x, y);
                    lay->SetSpatialFilter(&pt);
                    // The returned feature becomes the responsibility of the caller to delete with OGRFeature::DestroyFeature().
                    poFeature = lay->GetNextFeature();
                    if (poFeature != NULL)
                    {
                        aPt->setCaPaKey(poFeature->GetFieldAsString("CaPaKey"));
                        aPt->setCaPaFID(poFeature->GetFID());
                        OGRFeature::DestroyFeature(poFeature);
                    }
                }
                GDALClose(mDS);
            }
            else
            {
                std::cout << inputPath << " n'existe pas ";
            }
        }
    }

    ptCadastre::ptCadastre(std::shared_ptr<cadastre> aCadastre, double x, double y) : mCad(aCadastre), mCommune(""), mFID(0), mCaPaKey("")
    {
        if (!boost::filesystem::exists(mCad->mDirBDCadastre))
        {
            std::cout << " bd cadastre " << mCad->mDirBDCadastre << " n'existe pas!! ça va planter ... \n\n\n\n"
                      << std::endl;
        }
        else
        {
            std::unique_ptr<dbo::backend::Sqlite3> sqlite3{new dbo::backend::Sqlite3(mCad->mDirBDCadastre)};

            if (globTest)
            {
                sqlite3->setProperty("show-queries", "true");
            }
            session.setConnection(std::move(sqlite3));
            session.mapClass<capa>("capa");
            mCad->getCaPa4pt(x, y, this);

            // maintenant je vais chercher l'objet mappé capa
            if (mCaPaKey != "")
            {
                dbo::ptr<capa> pt = mCad->getCaPaPtr(mCaPaKey, &session);
                mCommune = mCad->Commune(pt->comINS);
            }
        }
    }

    std::string ptCadastre::displayAllInfoInOverlay()
    {
        return "parcelle cadastrale " + mCaPaKey + ", commune de " + mCommune;
    }

    void ptCadastre::usePolyg4Stat()
    {
        std::cout << "ptCadastre::usePolyg4Stat" << std::endl;
        geoJson_.emit(mCad->createPolygonPaCa(mFID), displayAllInfoInOverlay());
    }

    /*
    void cadastre::writeArchive(std::string aArchive){
        std::cout << "Ecriture de l'archive des parcelles cadastrales dans " << aArchive << std::endl;
        std::ofstream ofs(aArchive);
        //boost::archive::xml_oarchive oa(ofs);
        //oa << boost::serialization::make_nvp("cadastre",*this);
        boost::archive::binary_oarchive oa(ofs);
        oa <<   boost::serialization::make_binary_object(&mVCaPa, sizeof(mVCaPa));
        ofs.close();
    }



    void capa::writeArchive(std::string aArchive){
        std::cout << "Ecriture de l'archive de la parcelle cadastrale dans " << aArchive << std::endl;
        std::ofstream ofs(aArchive);
        boost::archive::binary_oarchive oa(ofs);
        oa <<   boost::serialization::make_binary_object(this, sizeof(*this));
        ofs.close();
    }



    cadastre::cadastre(std::string xmlCadastre){
    std::cout << "Lecture de l'archive du cadastre "<< std::endl;
     std::ifstream ifs(xmlCadastre);
     boost::archive::binary_iarchive archive(ifs);
    //archive & boost::serialization::make_nvp("cadastre",*this);
     ifs.close();
     std::cout << "Done "<< std::endl;
    }
    */
