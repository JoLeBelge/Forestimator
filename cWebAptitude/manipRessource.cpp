#include "manipRessource.h"

extern bool globTest;
extern std::string columnPath;

//./manip --BD "/home/jo/app/Forestimator/carteApt/data/aptitudeEssDB.db" --colPath "Dir3" --test 1

// pour manipuler les xml avec les ressources des 2 sites web, un soft dédié

int main(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")
            ("BD", po::value<std::string>(), "chemin d'accès à la BD forestimator")
            ("colPath", po::value<std::string>(), "nom de la colonne de fichierGIS et layerApt propre à la machine (chemin d'accès couche en local)")
            ;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("test")) {globTest=vm["test"].as<bool>();}

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
    std::string aBD("");
    if (vm.count("BD")) {aBD=vm["BD"].as<std::string>();std::cout << " chemin bd =" << aBD << std::endl;}
    if (vm.count("colPath")) {columnPath=vm["colPath"].as<std::string>();std::cout << " colPath =" << columnPath << std::endl;}
    cDicoApt *dico=new cDicoApt(aBD);

    std::cout << " export des messages xml \n\n\n" << std::endl;
    std::ifstream theFile;
    std::string aFile=dico->File("TMPDIR")+"traductionPhytospy.xml";
    //theFile.open("/home/jo/app/phytospy/data/phytoTool.xml");
     theFile.open("/home/jo/Téléchargements/new.xml");

    std::ofstream aOut;
    aOut.open(aFile,std::ios::out);

    Wt::WMessageResourceBundle resource;
    resource.use("/home/jo/Téléchargements/new");


    // lecture du xml
    xml_document<> doc;
    xml_node<> * root_node;
    std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    doc.parse<0>(&buffer[0]);
    // Find our root node
    root_node = doc.first_node("messages");
    for (xml_node<> * node = root_node->first_node("message"); node; node = node->next_sibling())
    {
        // il faudrait tester si l'attribut id existe, sinon plante. pour le moment c'est pas fonctionnel
        if (node->first_attribute("id")->value()){
            std::string aId(node->first_attribute("id")->value());
            //std::string aMessage(node->value());
            //std::cout << "message value = "<< aMessage << std::endl;
            Wt::LocalizedString ls=resource.resolveKey("fr",aId);
            std::string aMessage= ls.value;

            // traduction par deepl
            std::string url("https://api-free.deepl.com/v2/translate?auth_key=fe374c38-a172-9620-b311-8f427be37e29:fx&target_lang=EN&source_lang=FR&tag_handling=html&text=");
            //"curl -H "Authorization: DeepL-Auth-Key fe374c38-a172-9620-b311-8f427be37e29:fx" https://api-free.deepl.com/v2/usage";
            CURL *curl;
            CURLcode res;
            std::string readBuffer;
            curl = curl_easy_init();
            if(curl) {
                // caractère accentué et espace ; passe pas sans le escape
                //boost::replace_all(aMessage,"&nbsp;","\n");// pas géré par easy_escape

                char *urlified = curl_easy_escape(curl, aMessage.c_str(), 0);
                //std::cout << "urlified " << urlified << std::endl;
                url+=urlified;
                std::cout << url << std::endl;
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
                res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);
                std::cout << "result has size " << readBuffer.size() << ", " << readBuffer << std::endl;
                //std::cout << "CURLcode res : " << res << std::endl; // 0 means OK
                // split response
                std::string token("");
                std::string delimiter = "text\":\"";
                if (readBuffer.find(delimiter)!=std::string::npos){
                    std::string token =readBuffer.substr(readBuffer.find(delimiter)+7,readBuffer.size());
                    token=token.substr(0,token.size()-4);
                    std::cout << token << std::endl;
                    boost::replace_all(token,"\\n","\n");
                    boost::replace_all(token,"\\\"","\"");
                    aOut <<"<message id=\""<< aId << "\">"<< token << "</message>\n\n" ;
                }

            } else {
                std::cout << "curl fonctionne pas" << std::endl;
            }


        } else {
            std::cout << "incorrect node " << std::endl;
        }
        //break;
    }

    doc.clear();
    theFile.close();
    aOut.close();
}
