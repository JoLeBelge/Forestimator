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
    desc.add_options()("help", "produce help message")("outil", po::value<int>(), "choix de l'outil à utiliser (1 : traduction des messages avec deepl, 2 : export de certaine map du dictionnaire sour forme de message xml, 3 : récupéré nom commun en anglais de la BD NCBI")("test", po::value<bool>(), "pour le test de nouvelles options en cours de développement")("BDapt", po::value<std::string>()->required(), "chemin d'accès à la BD forestimator")("BDphyto", po::value<std::string>()->required(), "chemin d'accès à la BD phyto")("colDir", po::value<std::string>()->required(), "nom de la colonne pour les chemins d'accès dans table fichierSite dans BD phyto")("colDirApt", po::value<std::string>(), "nom de la colonne pour les chemins d'accès dans  les tables fichierGIS dans BD Apt (facultatif)");
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
    if (vm.count("test"))
    {
        globTest = vm["test"].as<bool>();
    }
    if (vm.count("colDir"))
    {
        globColumnDir = vm["colDir"].as<std::string>();
    }
    if (vm.count("colDirApt"))
    {
        columnPath = vm["colDirApt"].as<std::string>();
    }

    // ici, créé mon dictionnaire et le mettre sous forme de membre dans resource
    std::string aBD("");
    if (vm.count("BDapt"))
    {
        aBD = vm["BDapt"].as<std::string>();
        std::cout << " chemin bd apt =" << aBD << std::endl;
    }
    if (vm.count("colPath"))
    {
        columnPath = vm["colPath"].as<std::string>();
        std::cout << " colPath =" << columnPath << std::endl;
    }
    cDicoApt *dico = new cDicoApt(aBD);

    std::string BDphyto("");
    if (vm.count("BDphyto"))
    {
        BDphyto = vm["BDphyto"].as<std::string>();
    }
    std::shared_ptr<cDicoPhyto> dicoPhyto = std::make_shared<cDicoPhyto>(BDphyto);

    if (vm.count("outil"))
    {
        int mode(vm["outil"].as<int>());
        switch (mode)
        {
        case 1:
        {
            std::cout << " deepl" << std::endl;
            deepl(dico);
            break;
        }
        case 2:
        {
            dicoToXml(dicoPhyto);
            break;
        }
        case 3:
        {
            processNCBI(dicoPhyto);
            break;
        }
        }
    }
}

void deepl(cDicoApt *dico)
{

    std::cout << " export des messages xml \n\n\n"
              << std::endl;
    std::string target_lang("NL");
    std::ifstream theFile;
    std::string aFile = dico->File("TMPDIR") + "phytoTool_" + target_lang + ".xml";

    // j'ai besoin du fichier ressource pour la resource, et pour parser avec rapidXML pour avoir les clés d'identifications
    std::string aXMLFile("/home/jo/app/phytospy/data/phytoTool.xml");
    aXMLFile = "/home/jo/Téléchargements/new.xml";
    theFile.open(aXMLFile);

    std::ofstream aOut;
    aOut.open(aFile, std::ios::out);

    Wt::WMessageResourceBundle resource;
    resource.use(aXMLFile.substr(0, aXMLFile.size() - 4));
    // lecture du xml
    xml_document<> doc;
    xml_node<> *root_node;
    std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    doc.parse<0>(&buffer[0]);
    // Find our root node
    root_node = doc.first_node("messages");
    for (xml_node<> *node = root_node->first_node("message"); node; node = node->next_sibling())
    {
        // il faudrait tester si l'attribut id existe, sinon plante. pour le moment c'est pas fonctionnel
        if (node->first_attribute("id")->value())
        {
            std::string aId(node->first_attribute("id")->value());
            // std::string aMessage(node->value());
            // std::cout << "message value = "<< aMessage << std::endl;
            Wt::LocalizedString ls = resource.resolveKey("fr", aId);
            std::string aMessage = ls.value;

            std::string en = traduction(aMessage, target_lang);
            aOut << "<message id=\"" << aId << "\">" << en << "</message>\n\n";
        }
        else
        {
            std::cout << "incorrect node " << std::endl;
        }
    }
    doc.clear();
    theFile.close();
    aOut.close();
}

void dicoToXml(std::shared_ptr<cDicoPhyto> dico)
{
    std::cout << " export des messages xml depuis le dico\n"
              << std::endl;
    std::string target_lang("NL");
    std::string aFile = dico->File("TMPDIR") + "messageFromBD.xml";
    std::string aFileEN = dico->File("TMPDIR") + "messageFromBD_" + target_lang + ".xml";
    std::ofstream aOut, aOutEN;
    aOut.open(aFile, std::ios::out);
    aOut << "<messages>\n";

    /*
    for (auto kv : dico->Dico_code2NomFR){
        std::string code=kv.first;
        std::string nomfr=kv.second;
        aOut <<"<message id=\""<< code << ".nom\">"<< nomfr << "</message>\n" ;
    }
    for (auto kv : dico->Dico_groupeNT){
        std::string code=kv.first;
        std::string nomfr=kv.second;
        aOut <<"<message id=\""<< code << ".nom\">"<< nomfr << "</message>\n" ;
    }
    for (auto kv :  dico->grEcos){
        std::string code=kv.first;
        grEco ge=kv.second;
        aOut <<"<message id=\""<< code << ".nomGroupe\">"<< ge.nom_groupe << "</message>\n" ;
    }*/
    aOut << "</messages>\n";
    aOut.close();

    // traduction anglaise ou néérlandaise
    aOutEN.open(aFileEN, std::ios::out);
    aOutEN << "<messages>\n";
    /*
    for (auto kv : dico->Dico_groupeNT){
            std::string code=kv.first;
            std::string nomfr=kv.second;
            std::string en=traduction(nomfr, target_lang);
            if (en!=""){
                aOutEN <<"<message id=\"<< code << ".nom\">"<< en << "</message>\n" ;
           }
    }*/
    for (auto kv : dico->grEcos)
    {
        std::string code = kv.first;
        grEco ge = kv.second;
        std::string aTrad("");
        if (target_lang == "EN")
        {
            aTrad = ge.nom_shortEN;
        }
        else
        {
            aTrad = traduction(ge.nom_groupe, target_lang);
        }

        aOutEN << "<message id=\"" << code << ".nomGroupe\">" << aTrad << "</message>\n";
    }

    /*
    for (auto kv : dico->Dico_code2NomFR){
        std::string code=kv.first;
        std::string nomfr=kv.second;
        std::string en=dico->nomEN(code);
        if (target_lang!="EN"){en="";}
        if (en==""){en=traduction(nomfr,target_lang);}
        if (en!=""){
            aOutEN <<"<message id=\""<< code << ".nom\">"<< en << "</message>\n" ;
        }
    }*/
    aOutEN << "</messages>\n";
    aOutEN.close();
}

std::string traduction(std::string afr, std::string target_lang)
{

    std::string token("");
    // traduction par deepl
    std::string url("https://api-free.deepl.com/v2/translate?auth_key=fe374c38-a172-9620-b311-8f427be37e29:fx&target_lang=" + target_lang + "&source_lang=FR&tag_handling=html&text=");
    //"curl -H "Authorization: DeepL-Auth-Key fe374c38-a172-9620-b311-8f427be37e29:fx" https://api-free.deepl.com/v2/usage";
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if (curl)
    {
        // caractère accentué et espace ; passe pas sans le escape
        // boost::replace_all(aMessage,"&nbsp;","\n");// pas géré par easy_escape

        char *urlified = curl_easy_escape(curl, afr.c_str(), 0);
        // Append escaped text if available, then free it with curl_free
        if (urlified != NULL)
        {
            url += urlified;
            curl_free(urlified);
        }
        else
        {
            std::cerr << "Warning: curl_easy_escape returned NULL, using unescaped text" << std::endl;
            url += afr; // fallback: append raw text (may break request)
        }
        std::cout << "url: \n"
                  << std::endl;
        std::cout << url << "\n"
                  << std::endl;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // Avoid blocking forever on slow or unresponsive endpoints
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // seconds to wait for connection
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);       // seconds max for the whole transfer
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
        // std::cout << "result has size " << readBuffer.size() << ", " << readBuffer << std::endl;
        // std::cout << "CURLcode res : " << res << std::endl; // 0 means OK
        //  split response
        std::string delimiter = "text\":\"";
        if (readBuffer.find(delimiter) != std::string::npos)
        {
            token = readBuffer.substr(readBuffer.find(delimiter) + 7, readBuffer.size());
            token = token.substr(0, token.size() - 4);
            std::cout << token << std::endl;
            boost::replace_all(token, "\\n", "\n");
            boost::replace_all(token, "\\\"", "\"");
            boost::replace_all(token, "\\t", "\t");
        }
    }
    return token;
}

// a fonctionné pour 471 plantes
void processNCBI(std::shared_ptr<cDicoPhyto> dico)
{
    std::cout << " get common name for plant species from ncbi data\n"
              << std::endl;
    // j'ai deux BDs sqlite, la BD phytospy, la bd avec une table que j'ai faite issue de la BD NCBI
    std::string pathBDNCBI = "/home/jo/Documents/Lisein_j/AC2016/RelFloristique/phyto2021/englishName/ndbi/namesNCBI.db";
    sqlite3 *db_, *dbOUT_;
    int rc;
    rc = sqlite3_open_v2(pathBDNCBI.c_str(), &db_, SQLITE_OPEN_READWRITE, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
        std::cout << pathBDNCBI << std::endl;
        std::cout << "result code : " << rc << std::endl;
    }
    else
    {

        rc = sqlite3_open_v2("/home/jo/app/phytospy/data/data/Phyto_NT_JL.db", &dbOUT_, SQLITE_OPEN_READWRITE, NULL);

        sqlite3_stmt *stmt, *stmt2, *stmt3;
        int c(0);
        for (auto kv : dico->Dico_code2Nom)
        {
            c++;
            std::string codeEs = kv.first;
            std::string scientificName = kv.second;
            const char *q1 = "SELECT id FROM names WHERE name=?;";
            if (sqlite3_prepare_v2(db_, q1, -1, &stmt, NULL) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, scientificName.c_str(), -1, SQLITE_TRANSIENT);
                while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (sqlite3_column_type(stmt, 0) != SQLITE_NULL)
                {
                    int id = sqlite3_column_int(stmt, 0);
                    // std::cout << "id of " << codeEs <<" is " << id << std::endl;
                    const char *q2 = "SELECT name FROM names WHERE id=? AND type='common name';";
                    if (sqlite3_prepare_v2(db_, q2, -1, &stmt2, NULL) == SQLITE_OK)
                    {
                        sqlite3_bind_int(stmt2, 1, id);
                        while (sqlite3_step(stmt2) == SQLITE_ROW)
                    {
                        if (sqlite3_column_type(stmt2, 0) != SQLITE_NULL)
                        {
                            std::string commonName = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt2, 0)));
                            std::cout << scientificName << " is " << commonName << std::endl;
                            // sauver le résultat dans la table
                            const char *q3 = "UPDATE dico_espece_all SET nom_en=? WHERE code_espece=?";
                            if (sqlite3_prepare_v2(dbOUT_, q3, -1, &stmt3, NULL) == SQLITE_OK)
                            {
                                sqlite3_bind_text(stmt3, 1, commonName.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_text(stmt3, 2, codeEs.c_str(), -1, SQLITE_TRANSIENT);
                                // applique l'update
                                sqlite3_step(stmt3);
                                sqlite3_finalize(stmt3);
                            }
                            else
                            {
                                int ecode = sqlite3_errcode(dbOUT_);
                                std::cout << "\n error code is " << ecode;
                            }
                            break;
                        }
                    }
                    sqlite3_finalize(stmt2);
                }
                break;
                }
                sqlite3_finalize(stmt);
                if (c % 500 == 0)
            {
                std::cout << c << "plantes effectués" << std::endl;
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db_);
        sqlite3_close(dbOUT_);
    }
}
