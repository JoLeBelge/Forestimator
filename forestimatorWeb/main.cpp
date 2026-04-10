#include "main.h"
extern bool globTest;
extern std::string columnPath;

int launchForestimator(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")("BD", po::value<std::string>(), "chemin d'accès à la BD forestimator")("colPath", po::value<std::string>(), "nom de la colonne de fichierGIS et layerApt propre à la machine (chemin d'accès couche en local)");
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("test"))
    {
        globTest = vm["test"].as<bool>();
    }

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
    std::string aBD("");
    if (vm.count("BD"))
    {
        aBD = vm["BD"].as<std::string>();
        if (globTest){std::cout << " chemin bd =" << aBD << std::endl;}
    }
    if (vm.count("colPath"))
    {
        columnPath = vm["colPath"].as<std::string>();
        if (globTest){std::cout << " colPath =" << columnPath << std::endl;}
    }
    cDicoApt *dico = new cDicoApt(aBD);

    std::shared_ptr<stationDescResource> resource = std::shared_ptr<stationDescResource>(new stationDescResource(dico));
    std::shared_ptr<rasterClipResource> rClipRaster = std::shared_ptr<rasterClipResource>(new rasterClipResource(dico));
    std::shared_ptr<anaPonctuelleResource> anaPonctResource = std::shared_ptr<anaPonctuelleResource>(new anaPonctuelleResource(dico));
    std::shared_ptr<anaSurfResource> anaSurfRes = std::shared_ptr<anaSurfResource>(new anaSurfResource(dico));
    std::shared_ptr<staticMapResource> smResource = std::shared_ptr<staticMapResource>(new staticMapResource(dico));
    std::shared_ptr<polygFromMobile> fromMobile = std::shared_ptr<polygFromMobile>(new polygFromMobile(dico->File("docroot") + "validCarteEss.db"));
    std::shared_ptr<voirieFromMobile> route = std::shared_ptr<voirieFromMobile>(new voirieFromMobile(dico->File("docroot") + "observationVoirie.db"));

    try
    {
        Wt::WServer server = Wt::WServer(argc, argv, WTHTTP_CONFIGURATION);

        // set first ressources with sub-folder /api/
        // then add entry point for the web site
        server.addResource(resource, "/api/${tool}/args/${toolarg}/polygon/${pol}");
        server.addResource(resource, "/api/${tool}/polygon/${pol}");
        server.addResource(resource, "/api/${tool}/point/${pt}");
        server.addResource(resource, "/api/${tool}/${pts}");
        server.addResource(resource, "/api/${tool}/args/${toolarg}/point/${pt}");
        // seule url de la ressource ou tout les arguments sont vide
        server.addResource(resource, "/api/help");

        // pour avoir la table dictionnaire
        server.addResource(resource, "/api/${tool}");

        // exemple http://localhost:8085/api/clipRast/layerCode/EP_FEE/xmin/200000.0/ymin/80000.0/xmax/250000.0/ymax/100000.0/toto.tif
        server.addResource(rClipRaster, "/api/clipRast/layerCode/${layerCode}/xmin/${xmin}/ymin/${ymin}/xmax/${xmax}/ymax/${ymax}");
        server.addResource(rClipRaster, "/api/rastPColor/layerCode/${layerCode}");
        // pour créer le raster avec palette de couleur (forestimator Mobile)

        server.addResource(smResource, "/api/staticMap/layerCode/${layerCode}/polygon/${pol}");
        server.addResource(smResource, "/api/staticMap/layerCode/${layerCode}/sz/${sz}/polygon/${pol}");
        server.addResource(smResource, "/api/staticMap/layerCode/${layerCode}/env/${env}/polygon/${pol}");
        server.addResource(smResource, "/api/staticMap/layerCode/${layerCode}/env/${env}/sz/${sz}/polygon/${pol}");

        // http://localhost:8085/api/anaPt/layers/EP_FEE+EP_CS+MNH2019+CNSW/x/200000.3/y/80000.1
        server.addResource(anaPonctResource, "/api/anaPt/layers/${listLayerCode}/x/${x}/y/${y}");

        server.addResource(anaSurfRes, "/api/anaSurf/layers/${listLayerCode}/polygon/${pol}");

        server.addResource(fromMobile, "/api/polygFromMobile/${feature}");
        server.addResource(route, "/api/voirieFromMobile/${feature}");

        // fileResource pour les cartes à l'échelle de toute la RW
        for (auto kv : dico->VlayerBase())
        {
            std::shared_ptr<layerBase> l = kv.second;
            std::string aCode = kv.first;
            if (l->rasterExist())
            {
                std::shared_ptr<layerResource> fileResource = std::shared_ptr<layerResource>(new layerResource(l,dico));
                fileResource->suggestFileName(l->NomFileWithExt());
                // if (globTest){std::cout << " ajout fileresource " << l->getPathTif() << ", nom fichier " <<  l->NomFileWithExt() << " sous url data/"<<aCode << std::endl;}
                server.addResource(fileResource, "/telechargement/" + aCode);
                // fichier de symbologie
                std::shared_ptr<layerResource> fileResource2 = std::shared_ptr<layerResource>(new layerResource(l, dico,1));
                fileResource2->suggestFileName(l->NomFile() + ".qml");
                server.addResource(fileResource2, "/telechargement/" + aCode + "qml");
                // table dictionnaire
                std::shared_ptr<layerResource> fileResource3 = std::shared_ptr<layerResource>(new layerResource(l,dico, 2));
                fileResource3->suggestFileName(l->NomFile() + "_dico.csv");
                server.addResource(fileResource3, "/telechargement/" + aCode + "dico");
            }
        }

        for (int i(1); i < 18; i++)
        {
            std::string aName("US-A" + std::to_string(i) + ".pdf");
            std::shared_ptr<Wt::WFileResource> fileResource3 = std::shared_ptr<Wt::WFileResource>(new Wt::WFileResource("application/pdf", dico->File("docroot") + "pdf/" + aName));
            fileResource3->suggestFileName(aName);
            server.addResource(fileResource3, "/telechargement/" + aName);
        }

        std::shared_ptr<Wt::WFileResource> fileResource = std::shared_ptr<Wt::WFileResource>(new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "observationVoirie.db"));
        fileResource->suggestFileName("observationVoirie.db");
        server.addResource(fileResource, "/telechargement/voirie");

        std::shared_ptr<Wt::WFileResource> fileResource5 = std::shared_ptr<Wt::WFileResource>(new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "OGF.db"));
        fileResource5->suggestFileName("OGF.db");
        server.addResource(fileResource5, "/telechargement/OGF");

        std::shared_ptr<Wt::WFileResource> fileResource6 = std::shared_ptr<Wt::WFileResource>(new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "validCarteEss.db"));
        fileResource6->suggestFileName("validCarteEss.db");
        server.addResource(fileResource6, "/telechargement/validCarteEss");

        server.addEntryPoint(Wt::EntryPointType::Application, std::bind(&createWebAptitudeApplication, std::placeholders::_1, dico));
        Session::configureAuth();
        server.run();
    }
    catch (Wt::WServer::Exception &e)
    {
        std::cerr << "sError" << e.what() << std::endl;
    }
    catch (Wt::Dbo::Exception &e)
    {
        std::cerr << "Dbo exception: " << e.what() << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
    }
    return 0;
}

std::unique_ptr<Wt::WApplication> createWebAptitudeApplication(const Wt::WEnvironment &env, cDicoApt *dico)
{

    if (env.internalPath() == "/documentation" || env.internalPath().substr(0, 14) == "/documentation")
    {
        ;
    }
    else if (env.internalPath() == "/cartographie")
    {
        ;
    }
    else if (env.internalPath() == "/resultat")
    {
        ;
    }
    else if (env.internalPath() == "/parametres")
    {
        ;
    }
    else if (env.internalPath() == "/" || env.internalPath() == "")
    {
        ;
    }
    else if (env.internalPath().rfind("/auth", 0) == 0)
    { /* authentification links ! */
        ;
    }
    else if (env.internalPath() == "/stats_analytics")
    {
        auto app = std::make_unique<PageAnalytics>(env, dico->File("docroot") + "analytics.db");
        return app;
    }
    else if (env.internalPath() == "/encodage.OGF")
    {
        auto app = std::make_unique<formOGF>(env, dico, dico->File("docroot") + "OGF.db");
        return app;
    }
   /* else if (env.internalPath() == "/encodage.terrain")
    {
        auto app = std::make_unique<encodageRelTerrain>(env, dico->File("docroot") + "ACR.db");
        return app;
    }*/
    else
    {
        std::cout << "internal path pas geré : " << env.internalPath() << std::endl;

        // stats trafic web
        // Analytics anal(dico->File("docroot")+"analytics.db");

        auto app404 = std::make_unique<Wt::WApplication>(env);
        auto theme = std::make_shared<Wt::WBootstrapTheme>();
        theme->setVersion(Wt::BootstrapVersion::v3);
        theme->setResponsive(true);
        app404->setInternalPathValid(false);
        app404->root()->addWidget(std::make_unique<Wt::WText>("ERREUR: Page introuvable..."));
        return app404;
    }
    return std::make_unique<cWebAptitude>(env, dico);
}

void layerResource::handleRequest(const Http::Request &request, Http::Response &response)
{

    std::string archiveName = ml->getPathTif();
    if (mQmlDico==1)
    {
        archiveName = ml->symbology();

    } else if (mQmlDico==2){
        // création du dictionnaire (fichier temporaire)
        boost::filesystem::path tmpPath = boost::filesystem::path(mDico->File("TMPDIR")) / boost::filesystem::unique_path("tmp-%%%%-%%%%-%%%%.csv");
        std::ofstream out(tmpPath, std::ios::app);
        out << "Valeur Numérique Raster;Signification\n";
        out << ml->getDicoValStr();
        out.close();
        archiveName = tmpPath.c_str();
    }

    std::ifstream r(archiveName.c_str(), std::ios::in | std::ios::binary);
    handleRequestPiecewise(request, response, r);
    r.close();
}

void ForestimatorMainTask::run()
{
    //double arr[4] = {1.,2.,3.,5.};
    //std::cout << arr[1] / 0. << std::endl;
    launchForestimator(*argc, *argv);
    return;
}

int main(int argc, char **argv)
{
    int nThreads = 2;
    pool = new Pool(new ForestimatorMainTask(&argc, &argv), nThreads);
    pool->start();
    std::cout << "Exit application" << std::endl;
    delete (pool);
}
