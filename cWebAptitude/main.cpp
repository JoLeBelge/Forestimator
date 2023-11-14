#include "main.h"
// Threadpool implementation TT
#include "./threadpool/Task.hpp"
#include "./threadpool/Pool.hpp"

static Pool* pool;
extern bool globTest;
extern std::string columnPath;

int launchForestimator(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")
            ("BD", po::value<std::string>(), "chemin d'accès à la BD forestimator")
            ("colPath", po::value<std::string>(), "nom de la colonne de fichierGIS et layerApt propre à la machine (chemin d'accès couche en local)")
            ;
    po::variables_map vm;
    //po::store(po::parse_command_line(argc, argv, desc), vm);
    //https://stackoverflow.com/questions/15552284/boostprogram-options-how-to-ignore-unknown-parameters
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("test")) {globTest=vm["test"].as<bool>();}

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
    std::string aBD("");
    if (vm.count("BD")) {aBD=vm["BD"].as<std::string>();std::cout << " chemin bd =" << aBD << std::endl;}
    if (vm.count("colPath")) {columnPath=vm["colPath"].as<std::string>();std::cout << " colPath =" << columnPath << std::endl;}
    cDicoApt *dico=new cDicoApt(aBD);

    stationDescResource resource(dico);
    try {
        Wt::WServer server{argc, argv, WTHTTP_CONFIGURATION};

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


        cnswresource cnswr(dico->File("TMPDIR")+"/");
        server.addResource(&cnswr, "/CNSW");

        // fileResource pour les cartes à l'échelle de toute la RW
        for (auto kv : dico->VlayerBase()){
            std::shared_ptr<layerBase> l= kv.second;
            std::string aCode=kv.first;
            if (l->rasterExist()){
            layerResource * fileResource = new layerResource(l);
            fileResource->suggestFileName(l->NomFileWithExt());
           // if (globTest){std::cout << " ajout fileresource " << l->getPathTif() << ", nom fichier " <<  l->NomFileWithExt() << " sous url data/"<<aCode << std::endl;}
            server.addResource(fileResource, "/telechargement/"+aCode);
            // fichier de symbologie
            layerResource * fileResource2 = new layerResource(l,1);
            fileResource2->suggestFileName(l->NomFile()+".qml");
            server.addResource(fileResource2, "/telechargement/"+aCode+"qml");
            }
        }

        server.addEntryPoint(Wt::EntryPointType::Application, std::bind(&createAuthApplication,std::placeholders::_1, dico));
        Session::configureAuth();

        server.run();
    } catch (Wt::WServer::Exception& e) {
        std::cerr << "sError" << e.what() << std::endl;
    } catch (Wt::Dbo::Exception &e) {
        std::cerr << "Dbo exception: " << e.what() << std::endl;
    } catch (std::exception &e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }
}

std::unique_ptr<Wt::WApplication> createAuthApplication(const Wt::WEnvironment &env, cDicoApt *dico)
{
    //std::cout << env.internalPath() << " " << env.deploymentPath() << std::endl;
    //std::cout << env.getParameter("a0") << std::endl;
    /*std::cout << "ffor: " << env.headerValue("X-Forwarded-For") << std::endl;
    std::cout << "fpro: " << env.headerValue("X-Forwarded-Prot") << std::endl;
    std::cout << "ip: " << env.headerValue("Client-IP") << std::endl;
    std::cout << "ua: " << env.headerValue("User-Agent") << std::endl;*/

    if (env.internalPath() == "/documentation" || env.internalPath().substr(0,14)== "/documentation"){
        ;
    }else if (env.internalPath() == "/cartographie"){
        ;
    }else if (env.internalPath() == "/resultat"){
        ;
    }else if (env.internalPath() == "/parametres"){
        ;
    }else if (env.internalPath() == "/" || env.internalPath() == ""){
        ;
    }else if (env.internalPath().rfind("/auth",0)==0){ /* authentification links ! */
        ;
    }else if (env.internalPath() == "/stats_analytics"){
        auto app = std::make_unique<PageAnalytics>(env,dico->File("docroot")+"analytics.db");
        return app;
    }else{
        std::cout << "internal path pas geré : " << env.internalPath() << std::endl;


        // stats trafic web
        Analytics anal(dico->File("docroot")+"analytics.db");

        auto app404 = std::make_unique<Wt::WApplication>(env);
        auto theme = std::make_shared<Wt::WBootstrapTheme>();
        theme->setVersion(Wt::BootstrapVersion::v3);
        theme->setResponsive(true);
        app404->setInternalPathValid(false);
        app404->root()->addWidget(std::make_unique<Wt::WText>("ERREUR: Page introuvable..."));
        return app404;
    }


    return std::make_unique<AuthApplication>(env,dico);
}

void layerResource::handleRequest(const Http::Request &request, Http::Response &response){
    //std::cout << "fichier " << ml->getPathTif() << std::endl;
    // création des archives zip avec la carte + la symbologie

    //std::string archiveName=ml->Dico()->File("OUTDIR")+ml->NomFile()+".zip";
    std::string archiveName=ml->getPathTif();
    if (mQml){archiveName = ml->symbology();}

    if (!boost::filesystem::exists(archiveName)){
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

<<<<<<< HEAD
class ForestimatorMainTask : public Task {
    int *argc;
    char ***argv;
    void run() override {
        launchForestimator(*argc, *argv);
        return;
    }
public:
    ForestimatorMainTask(int *argc, char ***argv) : argc(argc), argv(argv){}
};
=======
void ForestimatorMainTask::run(){
        launchForestimator(*argc, *argv);
        return;
    }
>>>>>>> b6b7b9d8b6002d737fe630d0fc2e28adae0a78b3

int main(int argc, char **argv){
    int nThreads = 1;
    pool = new Pool(new ForestimatorMainTask(&argc, &argv), nThreads);
    pool->start();
    delete(pool);
}

