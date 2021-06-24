/*
 * Copyright (C) 2008 Emweb bvba, Kessel-Lo, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "main.h"
//#include "auth.h"


/* attention, je n'ai jamais réussi à paramètrer deux docroot donc je dois tout mettre dans un seul et unique!
 *
 * ./WebAptitude --http-address=0.0.0.0 --http-port=8085 --deploy-path=/WebAptitude --docroot="./" --config="/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude/wt_config.xml"
 * Current arg :
 * ./WebAptitude --deploy-path=/ --docroot "/data1/Forestimator/build-WebAptitude;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
 * sudo ./WebAptitude --deploy-path=/ --docroot "/home/sam/master_chatmetaleux/Forestimator/data;favicon.ico,/resources,/style,/tmp,/data,/Tuiles" --http-port 80 --http-addr 0.0.0.0
 * ./WebAptitude --deploy-path=/ --docroot "/home/lisein/Documents/carteApt/Forestimator/data/;/favicon.ico,/js,/jslib,/resources,/style,/tmp,/data,/Tuiles" --http-port 8085 --http-addr 0.0.0.0
 * Ajout du wt config.xml :
 * sudo ./WebAptitude --deploy-path=/ --docroot "/home/sam/master_chatmetaleux/Forestimator/data/;favicon.ico,/resources,/style,/tmp,/data,/js,/jslib" --http-port 80 --http-addr 0.0.0.0 -c ../data/wt_config.xml
*/

extern bool globTest;

int main(int argc, char **argv)
{
     std::cout << "création des options du programme." << std::endl;
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement");
    po::variables_map vm;
    //po::store(po::parse_command_line(argc, argv, desc), vm);
    //https://stackoverflow.com/questions/15552284/boostprogram-options-how-to-ignore-unknown-parameters
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
     if (vm.count("test")) {globTest=vm["test"].as<bool>();}

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
      std::cout << "loadBDpath et cree dico.." << std::endl;
    std::string aBD=loadBDpath();
    cDicoApt *Dico=new cDicoApt(aBD);
    stationDescResource resource(Dico);
  try {
    Wt::WServer server{argc, argv, WTHTTP_CONFIGURATION};

    // set first ressources with sub-folder /api/
    // then add entry point for the web site

    server.addResource(&resource, "/api/${tool}/args/${toolarg}/polygon/${pol}");
    server.addResource(&resource, "/api/${tool}/polygon/${pol}");
    // seul url de la ressource ou tout les arguments sont vide
    server.addResource(&resource, "/api/help");

    //j'ajouterai bien une autre ressource qui permet de visualiser le contenu des tables dictionnaire de la BD. Mais alors créer une classe qui lit les tables de la BD et qui les affiche sur demande
    // l'idée c'est que le programmeur puisse avoir accès au tables dictionnaires (à jours) sans que cela ne m'oblique à les lui envoyer.
    // ou alors juste visualiser le dictionnaire d'une layerBase? Est-ce que cela sera suffisant? facile à faire en tout cas...
    // https://redmine.webtoolkit.eu/boards/2/topics/17226?r=17243#message-17243
    // see paypal example

    server.addEntryPoint(Wt::EntryPointType::Application, std::bind(&createAuthApplication,std::placeholders::_1, Dico));

    Session::configureAuth();

    server.run();
  } catch (Wt::WServer::Exception& e) {
    std::cerr << e.what() << std::endl;
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
    //std::cout << env.getParameter("a1") << std::endl;
    if (env.internalPath() == "/documentation" | env.internalPath().substr(0,14)== "/documentation"){
        ;
    }else if (env.internalPath() == "/home"){
        ;
    }else if (env.internalPath() == "/cartographie"){
        ;
    }else if (env.internalPath() == "/analyse"){
        ;
    }else if (env.internalPath() == "/point"){
        ;
    }else if (env.internalPath() == "/resultat"){
        ;
    }else if (env.internalPath() == "/parametres"){
        ;
    }else if (env.internalPath() == "/" || env.internalPath() == ""){
        ;
    }else{
        std::cout << "internal path pas geré : " << env.internalPath() << std::endl;
        auto app404 = Wt::cpp14::make_unique<Wt::WApplication>(env);
        app404->setInternalPathValid(false);
        return app404;
    }


    return Wt::cpp14::make_unique<AuthApplication>(env,dico);
}
