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



int main(int argc, char **argv)
{

    stationDescResource resource;
  try {
    Wt::WServer server{argc, argv, WTHTTP_CONFIGURATION};

    // le probleme quand on combine l'application + des resources, c'est que pour activer l'application, il faut obligatoirement entre la racine de l'adresse
    //http://localhost:8085/ --> ok, j'ai forestimator
    //http://localhost:8085/presentation --> not found grr ça va pas du coup
    server.addResource(&resource, "${tool}/args/${toolarg}/polygon/${pol}");
    server.addResource(&resource, "${tool}/polygon/${pol}");

    server.addEntryPoint(Wt::EntryPointType::Application, createAuthApplication);

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

std::unique_ptr<Wt::WApplication> createAuthApplication(const Wt::WEnvironment &env)
{
    //std::cout << env.internalPath() << " " << env.deploymentPath() << std::endl;
    //std::cout << env.getParameter("a0") << std::endl;
    //std::cout << env.getParameter("a1") << std::endl;
    //if(env.internalPath()="")

    return Wt::cpp14::make_unique<AuthApplication>(env);
}
