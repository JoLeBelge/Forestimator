#include "manipRessource.h"

extern bool globTest;
extern std::string columnPath;
extern std::string globColumnDir;
extern std::string columnPath;

//./manip --outil 2 --BDphyto "/home/jo/app/phytospy/data/data/Phyto_NT_JL.db" --BDapt "/home/jo/app/Forestimator/carteApt/data/aptitudeEssDB.db" --colDir dir3 --colDirApt "Dir3"


// pour manipuler les xml avec les ressources des 2 sites web, un soft dédié

int main(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("outil", po::value<int>(), "choix de l'outil à utiliser (1 : traduction des messages avec deepl, 2 : export de certaine map du dictionnaire sour forme de message xml")
            ("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")
            ("BDapt", po::value<std::string>()->required(), "chemin d'accès à la BD forestimator")
            ("BDphyto", po::value<std::string>()->required(), "chemin d'accès à la BD phyto")
            ("colDir", po::value<std::string>()->required(), "nom de la colonne pour les chemins d'accès dans table fichierSite dans BD phyto")
            ("colDirApt", po::value<std::string>(), "nom de la colonne pour les chemins d'accès dans  les tables fichierGIS dans BD Apt (facultatif)");
    ;
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("test")) {globTest=vm["test"].as<bool>();}
    if (vm.count("colDir")) {globColumnDir=vm["colDir"].as<std::string>();}
    if (vm.count("colDirApt")) {columnPath=vm["colDirApt"].as<std::string>();}

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
    std::string aBD("");
    if (vm.count("BDapt")) {aBD=vm["BDapt"].as<std::string>();std::cout << " chemin bd apt =" << aBD << std::endl;}
    if (vm.count("colPath")) {columnPath=vm["colPath"].as<std::string>();std::cout << " colPath =" << columnPath << std::endl;}
    cDicoApt *dico=new cDicoApt(aBD);

    std::string BDphyto("");
    if (vm.count("BDphyto")) {BDphyto=vm["BDphyto"].as<std::string>();}
    std::shared_ptr<cDicoPhyto> dicoPhyto = std::make_shared<cDicoPhyto>(BDphyto);

    if (vm.count("outil")) {
        int mode(vm["outil"].as<int>());
        switch (mode) {
        case 1:{
            std::cout << " deepl" << std::endl;
            deepl(dico);
            break;
        }
        case 2:{
            dicoToXml(dicoPhyto);
            break;
        }
        }
    }

}


void deepl(cDicoApt *dico){

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

            std::string en=traduction(aMessage);
            aOut <<"<message id=\""<< aId << "\">"<< en << "</message>\n\n" ;

        } else {
            std::cout << "incorrect node " << std::endl;
        }
    }
    doc.clear();
    theFile.close();
    aOut.close();
}

void dicoToXml(std::shared_ptr<cDicoPhyto> dico){
    std::cout << " export des messages xml depuis le dico\n" << std::endl;
    std::string aFile=dico->File("TMPDIR")+"messageFromBD.xml";
    std::string aFileEN=dico->File("TMPDIR")+"messageFromBD_en.xml";
    std::ofstream aOut, aOutEN;
    aOut.open(aFile,std::ios::out);
    aOut <<"<messages>\n" ;

    /*for (auto kv : dico->Dico_code2NomFR){
        std::string code=kv.first;
        std::string nomfr=kv.second;
        aOut <<"<message id=\""<< code << ".nom\">"<< nomfr << "</message>\n" ;
    }*/
    for (auto kv : dico->Dico_groupeNT){
            std::string code=kv.first;
            std::string nomfr=kv.second;
            aOut <<"<message id=\""<< code << ".nom\">"<< nomfr << "</message>\n" ;
    }
    for (auto kv :  dico->grEcos){
          std::string code=kv.first;
          grEco ge=kv.second;
          aOut <<"<message id=\""<< code << ".nomGroupe\">"<< ge.nom_groupe << "</message>\n" ;
    }
    aOut <<"</messages>\n" ;
    aOut.close();

    // traduction anglaise
    aOutEN.open(aFileEN,std::ios::out);
    aOutEN <<"<messages>\n" ;
    /*for (auto kv : dico->Dico_groupeNT){
            std::string code=kv.first;
            std::string nomfr=kv.second;
            std::string en=traduction(nomfr);
            if (en!=""){
                aOutEN <<"<message id=\"groupeNT."<< code << "\">"<< en << "</message>\n" ;
           }
    }*/
    for (auto kv :  dico->grEcos){
          std::string code=kv.first;
          grEco ge=kv.second;
          aOutEN <<"<message id=\""<< code << ".nomGroupe\">"<< ge.nom_shortEN << "</message>\n" ;
    }

    /*
    for (auto kv : dico->Dico_code2NomFR){
        std::string code=kv.first;
        std::string nomfr=kv.second;
        std::string en=traduction(nomfr);
        if (en!=""){
            aOutEN <<"<message id=\""<< code << ".nom\">"<< en << "</message>\n" ;
        }
    }*/
    aOutEN <<"</messages>\n" ;
    aOutEN.close();
}

std::string traduction(std::string afr){

    std::string token("");
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

        char *urlified = curl_easy_escape(curl, afr.c_str(), 0);
        //std::cout << "urlified " << urlified << std::endl;
        url+=urlified;
        //std::cout << url << std::endl;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        //std::cout << "result has size " << readBuffer.size() << ", " << readBuffer << std::endl;
        //std::cout << "CURLcode res : " << res << std::endl; // 0 means OK
        // split response
        std::string delimiter = "text\":\"";
        if (readBuffer.find(delimiter)!=std::string::npos){
            token =readBuffer.substr(readBuffer.find(delimiter)+7,readBuffer.size());
            token=token.substr(0,token.size()-4);
            std::cout << token << std::endl;
            boost::replace_all(token,"\\n","\n");
            boost::replace_all(token,"\\\"","\"");
        }
    }
    return token;
}

