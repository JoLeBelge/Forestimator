#include "main.h"
extern bool globTest;
extern std::string columnPath;

int launchForestimator(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")("BD", po::value<std::string>(), "chemin d'accès à la BD forestimator")("colPath", po::value<std::string>(), "nom de la colonne de fichierGIS et layerApt propre à la machine (chemin d'accès couche en local)");
    po::variables_map vm;
    // po::store(po::parse_command_line(argc, argv, desc), vm);
    // https://stackoverflow.com/questions/15552284/boostprogram-options-how-to-ignore-unknown-parameters
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
        std::cout << " chemin bd =" << aBD << std::endl;
    }
    if (vm.count("colPath"))
    {
        columnPath = vm["colPath"].as<std::string>();
        std::cout << " colPath =" << columnPath << std::endl;
    }
    cDicoApt *dico = new cDicoApt(aBD);

    stationDescResource resource(dico);
    rasterClipResource rClipRaster(dico);
    anaPonctuelleResource anaPonctResource(dico);
    anaSurfResource anaSurfResource(dico);
    staticMapResource smResource(dico);

    try
    {
        Wt::WServer server = Wt::WServer(argc, argv, WTHTTP_CONFIGURATION);

        // set first ressources with sub-folder /api/
        // then add entry point for the web site
        server.addResource(&resource, "/api/${tool}/args/${toolarg}/polygon/${pol}");
        server.addResource(&resource, "/api/${tool}/polygon/${pol}");
        server.addResource(&resource, "/api/${tool}/point/${pt}");
        server.addResource(&resource, "/api/${tool}/args/${toolarg}/point/${pt}");
        // seule url de la ressource ou tout les arguments sont vide
        server.addResource(&resource, "/api/help");

        // pour avoir la table dictionnaire
        server.addResource(&resource, "/api/${tool}");

        // exemple http://localhost:8085/api/clipRast/layerCode/EP_FEE/xmin/200000.0/ymin/80000.0/xmax/250000.0/ymax/100000.0/toto.tif
        server.addResource(&rClipRaster, "/api/clipRast/layerCode/${layerCode}/xmin/${xmin}/ymin/${ymin}/xmax/${xmax}/ymax/${ymax}");
        server.addResource(&rClipRaster, "/api/rastPColor/layerCode/${layerCode}");
        // pour créer le raster avec palette de couleur (forestimator Mobile)

        server.addResource(&smResource, "/api/staticMap/layerCode/${layerCode}/polygon/${pol}");
        server.addResource(&smResource, "/api/staticMap/layerCode/${layerCode}/sz/${sz}/polygon/${pol}");
        server.addResource(&smResource, "/api/staticMap/layerCode/${layerCode}/env/${env}/polygon/${pol}");
        server.addResource(&smResource, "/api/staticMap/layerCode/${layerCode}/env/${env}/sz/${sz}/polygon/${pol}");

        // http://localhost:8085/api/anaPt/layers/EP_FEE+EP_CS+MNH2019+CNSW/x/200000.3/y/80000.1
        server.addResource(&anaPonctResource, "/api/anaPt/layers/${listLayerCode}/x/${x}/y/${y}");

        server.addResource(&anaSurfResource, "/api/anaSurf/layers/${listLayerCode}/polygon/${pol}");

        // fileResource pour les cartes à l'échelle de toute la RW
        for (auto kv : dico->VlayerBase())
        {
            std::shared_ptr<layerBase> l = kv.second;
            std::string aCode = kv.first;
            if (l->rasterExist())
            {
                layerResource *fileResource = new layerResource(l);
                fileResource->suggestFileName(l->NomFileWithExt());
                // if (globTest){std::cout << " ajout fileresource " << l->getPathTif() << ", nom fichier " <<  l->NomFileWithExt() << " sous url data/"<<aCode << std::endl;}
                server.addResource(fileResource, "/telechargement/" + aCode);
                // fichier de symbologie
                layerResource *fileResource2 = new layerResource(l, 1);
                fileResource2->suggestFileName(l->NomFile() + ".qml");
                server.addResource(fileResource2, "/telechargement/" + aCode + "qml");
            }
        }

        for (int i(1); i < 18; i++)
        {
            std::string aName("US-A" + std::to_string(i) + ".pdf");
            Wt::WFileResource *fileResource3 = new Wt::WFileResource("application/pdf", dico->File("docroot") + "pdf/" + aName);
            fileResource3->suggestFileName(aName);
            server.addResource(fileResource3, "/telechargement/" + aName);
        }

        Wt::WFileResource *fileResource3 = new Wt::WFileResource("application/pdf", dico->File("docroot") + "pdf/methodoInventaireTerrainJeunePeup.pdf");
        fileResource3->suggestFileName("methodoInventaireAncienneCoupeRase.pdf");
        server.addResource(fileResource3, "/telechargement/methodoInventaireACR");

        Wt::WFileResource *fileResource = new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "ACR.db");
        fileResource->suggestFileName("ACR.db");
        server.addResource(fileResource, "/telechargement/ACR");
        Wt::WFileResource *fileResource2 = new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "desserteForest.db");
        fileResource2->suggestFileName("desserteForest.db");
        server.addResource(fileResource2, "/telechargement/desserteForest");

        Wt::WFileResource *fileResource4 = new Wt::WFileResource("application/pdf", dico->File("docroot") + "pdf/invitationCarrefourForestier2025.pdf");
        fileResource4->suggestFileName("invitationCarrefourForestier2025.pdf");
        server.addResource(fileResource4, "/CF");

        Wt::WFileResource *fileResource5 = new Wt::WFileResource("application/x-sqlite3", dico->File("docroot") + "OGF.db");
        fileResource5->suggestFileName("OGF.db");
        server.addResource(fileResource5, "/telechargement/OGF");

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
}

std::unique_ptr<Wt::WApplication> createWebAptitudeApplication(const Wt::WEnvironment &env, cDicoApt *dico)
{
    // std::cout << env.internalPath() << " " << env.deploymentPath() << std::endl;
    // std::cout << env.getParameter("a0") << std::endl;
    /*std::cout << "ffor: " << env.headerValue("X-Forwarded-For") << std::endl;
    std::cout << "fpro: " << env.headerValue("X-Forwarded-Prot") << std::endl;
    std::cout << "ip: " << env.headerValue("Client-IP") << std::endl;
    std::cout << "ua: " << env.headerValue("User-Agent") << std::endl;*/

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
        /*   }else if (env.internalPath() == "/encodage.coupe.rase"){
               auto app = std::make_unique<formVielleCoupeRase>(env,dico, dico->File("docroot")+"ACR.db");
               return app;
           }else if (env.internalPath() == "/encodage.desserteForest"){
                   auto app = std::make_unique<formDesserteForest>(env,dico, dico->File("docroot")+"desserteForest.db");
                   return app;*/
    }
    else if (env.internalPath() == "/encodage.terrain")
    {
        auto app = std::make_unique<encodageRelTerrain>(env, dico->File("docroot") + "ACR.db");
        return app;
    }
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
    return Wt::cpp14::make_unique<cWebAptitude>(env, dico);
}

void layerResource::handleRequest(const Http::Request &request, Http::Response &response)
{
    // std::cout << "fichier " << ml->getPathTif() << std::endl;
    //  création des archives zip avec la carte + la symbologie

    // std::string archiveName=ml->Dico()->File("OUTDIR")+ml->NomFile()+".zip";
    std::string archiveName = ml->getPathTif();
    if (mQml)
    {
        archiveName = ml->symbology();
    }

    if (!boost::filesystem::exists(archiveName))
    {
        /*if (globTest){std::cout << "create archive pour raster complet " << std::endl;}
        ZipArchive* zf = new ZipArchive(archiveName);
        zf->open(ZipArchive::WRITE);
        zf->addFile(ml->NomFileWithExt(),ml->getPathTif());
        if (ml->hasSymbology()){zf->addFile(ml->NomFile()+".qml",ml->symbology());}
        zf->close();
        delete zf;
        }*/
    }

    std::ifstream r(archiveName.c_str(), std::ios::in | std::ios::binary);

    handleRequestPiecewise(request, response, r);
}

void ForestimatorMainTask::run()
{
    launchForestimator(*argc, *argv);
    return;
}

int main(int argc, char **argv)
{
    int nThreads = 1;
    pool = new Pool(new ForestimatorMainTask(&argc, &argv), nThreads);
    pool->start();
    std::cout << "Exit application" << std::endl;
    delete (pool);
}
