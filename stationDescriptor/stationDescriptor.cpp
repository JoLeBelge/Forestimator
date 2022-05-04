#include <iostream>

using namespace std;

#include "cdicoaptbase.h"
#include "cnsw.h"
#include "cdicoapt.h"

#include "boost/program_options.hpp"
#include "plaiprfw.h"
#include "layerbase.h"
#include <string>
#include <random>
#include <omp.h>
#include <iomanip>
namespace po = boost::program_options;
extern string dirBD;
std::string datDir("/home/lisein/Documents/Scolyte/projetRegioWood/Data/");
std::string dirIRMMap="/home/lisein/Documents/Scolyte/Data/climat/IRM/irmCarte/";
std::string dirMapDescr="toto";

namespace bf =boost::filesystem;

bool climat(0);
std::vector<int> vYears={2016,2017,2018,2019,2020};
std::vector<int> vMonths={3,4,5,6,7,8,9};
std::vector<std::string> vVAR={"Tmean","Tmax","Tmin","ETP","P","R","DJ"};
int globSeuilPres(70);

/*
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include "html.h"
#include "HTMLControl.h"
#include "HTMLDriver.h"
#include "iconvstream.h"
#include "format.h"
*/



/* jo 2020
Projet RégioWood 2 et thèse de Arthur G.
J'aimerai avoir un code c++ avec gdal qui effectue une description stationnelle au départ d'un shapefile.
En gros la même chose que je faisais déjà avec la librairie micmac mais en plus propre et avec gdal, meilleure portabilité
Je vais integrer mon dictionnaire Apt comme cela j'aurais accès au dictionnaire des cartes raster, au aptitude, au dictionnaire cnsw

j'ai besoin de calculer le NH maj, NT majoritaire, aptitude, hdom, Zbio, SS, AE, Topo, cnsw : drainage, prof sol, texture

2) thèse MP et démarche similaire mais avec Catalogues Station en premier plan, MNH2014 + MNH2019, probabilité HE
le masque de forêt Hetraie mature sert pour la selection de nos tuile
*/

void descriptionStation(std::string aShp);

OGRPoint * getCentroid(OGRPolygon * hex);

void echantillonTuiles(std::string aShp);

// découpe de polygone en tuile
void tuilage(std::string aShp);

// renvoie une grille de point, centre de tuile
std::vector<OGRPoint> squarebin(OGRLayer * lay, int rectSize);
// renvoie les tuiles carrées
std::vector<OGRGeometry*> squarebinPol(OGRLayer * lay, int rectSize);

// générer une grille depuis un raster masque, on garde les tuiles qui ont plus de x pct d'occurence de 1 dans le masque
void tuilageFromRaster(rasterFiles * rasterMask, int rectSize, double prop, double seuilRasterIn);

// stat sur un raster scolyte et ajout d'un champ dans shp tuile
void anaScolyteOnShp(rasterFiles * raster, std::string aShp);

int main(int argc, char *argv[])
{

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("outil", po::value<int>(), "choix de l'outil à utiliser (1 : description stationnelle sur un shapefile de polygone, 101 : ajoute un champ aux shapefile avec la valeur extraite d'un raster donné en entrée), 102 : création d'un shp tuile à partir d'un raster (RasterMasq) ")
            ("shp", po::value< std::string>(), "shapefile des polgones sur lesquels effectuer l'analyse surfacique")
            ("meteo", po::value<bool>(), "description de la station avec des indices météo en plus (précision chemin d'accès au cartes avec dirIRMMap)")
            ("dirIRMMap", po::value<std::string>(), "chemin d'accès aux cartes de l'IRM")
            ("dirBDForestimator", po::value<std::string>(), "chemin d'accès à la BD de forestimator, dont une table sert à avoir tout les chemins d'accès aux raster + cnsw qui nous sont nécessaire.")
            ("dirMapDescr", po::value<std::string>(), "pour outil 1. Dossier contenant des cartes de variables continues pour lesquelles on souhaite calculer la valeur moyenne sur la tuile.")
            ("raster", po::value< std::string>(), "raster de description du mileu pour ajout d'un champs dans shp (outil 101")
            ("rasterMask", po::value< std::string>(), "raster masque à partir duquel on va générer des tuiles (outil 102) , pour tuilages scolytes ou hetraie mature")
            ("seuilPP", po::value<int>(), "seuil de probabilité de présence, utilisé pour masquer le raster pour création de tuile (102), défaut 70")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }

    if (vm.count("outil")) {
        GDALAllRegister();


        int mode(vm["outil"].as<int>());
        switch (mode) {
        case 1:{
            std::cout << " description station sur base d'un shp de polygone" << std::endl;
            if (vm.count("shp")) {
                std::string file(vm["shp"].as<std::string>());
                if (vm.count("meteo")){climat =vm["meteo"].as<bool>();}

                if (vm.count("dirIRMMap")){dirIRMMap =vm["dirIRMMap"].as<std::string>();}
                if (vm.count("dirBDForestimator")){dirBD =vm["dirBDForestimator"].as<std::string>();}
                if (vm.count("dirMapDescr")){dirMapDescr =vm["dirMapDescr"].as<std::string>();}

                descriptionStation(file);


            } else {
                std::cout << "vous devez obligatoirement renseigner le shapefile en entrée avec l'argument shp" << std::endl;
            }
            break;
        }
        case 10:{
            std::cout << "choix placettes IPRFW et création d'un shp avec limite des placettes " << std::endl;
            // ouverture de la bd sqlite , requete convertie de xls en sqlite
            std::string requete("/home/lisein/Documents/Scolyte/projetRegioWood/Data/iprfw.db");
            int rc;
            sqlite3 * db_;
            rc = sqlite3_open(requete.c_str(), &db_);
            if( rc!=0) {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_));
                std::cout << requete << std::endl;
                std::cout << "result code : " << rc << std::endl;
            } else {

                std::ofstream aOut(datDir+"placetteEP.csv");
                aOut.precision(3);
                aOut << "IGN;NPL;X;Y;GHA_tot;PCT_EP;Age2020;SI\n";

                sqlite3_stmt * stmt;
                std::string SQLstring="SELECT IGN, NPL FROM plot;";
                sqlite3_prepare( db_, SQLstring.c_str(), -1, &stmt, NULL );//preparing the statement
                while(sqlite3_step(stmt) == SQLITE_ROW)
                {
                    if (sqlite3_column_type(stmt, 0)!=SQLITE_NULL && sqlite3_column_type(stmt, 1)!=SQLITE_NULL){
                        int ign=sqlite3_column_int( stmt, 0 );
                        int npl=sqlite3_column_int( stmt, 1 );
                        // instantie la classe pla
                        plaIPRFW p(ign,npl,db_);
                        if (p.isPessiere()){
                            // ajout de la placette au fichier txt

                            aOut << p.summary() << "\n";
                        }
                    }
                }
                sqlite3_finalize(stmt);

                aOut.close();
            }
            sqlite3_close(db_);

            break;
        }
            // fee lu ; je veux découper des polygones en tuile
        case 100:{
            if (vm.count("shp")) {
                std::string file(vm["shp"].as<std::string>());
                tuilage(file);
            }
            break;
        }
        case 104:{
            if (vm.count("shp")) {
                std::string fileShp(vm["shp"].as<std::string>());
                echantillonTuiles(fileShp);
            }
            break;
        }
        case 103:{
            if (vm.count("rasterMask") && vm.count("shp")) {
                std::string fileShp(vm["shp"].as<std::string>());
                std::string file(vm["rasterMask"].as<std::string>());
                // raster présence de scolyte
                rasterFiles r(file,"toto");
                anaScolyteOnShp(&r,fileShp);
            }
            break;
        }
            // Marie pierre; je veux créer des tuiles à partir d'un masque de hetraie mature
        case 102:{
            if (vm.count("seuilPP")) {globSeuilPres=vm["seuilPP"].as<int>();}
            if (vm.count("rasterMask")) {
                std::string file(vm["rasterMask"].as<std::string>());
                rasterFiles r(file,"toto");
                //tuilageFromRaster(&r,30,65.0); // 65 pct ça fait 6 pixel sur 9 Marie pierre
                // scolyte
                tuilageFromRaster(&r,50,65.0,globSeuilPres);
            }
            break;
        }
        case 101:{
            // ajoute un champ aux shapefile avec la valeur extraite de raster donné en entrée

            if (vm.count("shp")) {
                std::string file(vm["shp"].as<std::string>());
                // extraction des valeurs de pentes
                if (vm.count("raster")) {
                    std::string fileRaster(vm["raster"].as<std::string>());
                    GDALAllRegister();
                    //ouverture du shp input
                    const char *inputPath=file.c_str();
                    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
                    if( mDS != NULL )
                    {
                        std::cout << "shp chargé " << std::endl;

                        rasterFiles r(fileRaster,"toto");

                        OGRLayer * lay = mDS->GetLayer(0);
                        if (lay->FindFieldIndex("slope",0)==-1){

                            OGRFieldDefn * oFLD(NULL);
                            oFLD= new OGRFieldDefn("slope",  OFTReal);
                            oFLD->SetJustify(OGRJustification::OJLeft);
                            lay->CreateField(oFLD);
                        }
                        std::cout << "champ créé " << std::endl;
                        OGRFeature *poFeature;
                        while( (poFeature = lay->GetNextFeature()) != NULL )
                        {
                            //std::cout << "calcul stat " << std::endl;
                            basicStat stat =r.computeBasicStatOnPolyg(poFeature->GetGeometryRef());
                            poFeature->SetField("slope",stat.getMeanDbl());// in mem object
                            lay->SetFeature(poFeature);
                        }
                        GDALClose(mDS);
                    }

                }

            }
            break;
        }

            // Marie pierre; je veux créer des tuiles à partir d'un masque de hetraie mature
        case 200:{

            // test html2text
            /*int mode = HTMLDriver::PRINT_AS_ASCII;
            iconvstream is;
            bool toto(false);
            int width(79);
            const char *input_file = "/home/jo/app/html2text/tests/montest.html";
            HTMLControl control(is, mode, false, input_file);
            HTMLDriver driver(control, is, toto, width, mode, toto);

            if (driver.parse() != 0)*/
            // ouverture du fichier xml et on retire les balises xml (pas celle html)


            //std::string aCommand="./html2text -from_encoding UTF8 "+std::to_string(mVProduts.at(0)->mXmin)+" "+std::to_string(mVProduts.at(0)->mYmin)+" "+std::to_string(mVProduts.at(0)->mXmax)+" "+std::to_string(mVProduts.at(0)->mYmax)+ " -t_srs EPSG:"+std::to_string(epsg)+" -ot Byte -overwrite -tr 10 10 "+ masqueRW+ " "+ out;
            //std::cout << aCommand << std::endl;
            //system(aCommand.c_str());
            /*std::string file(vm["shp"].as<std::string>());
            xml_document<> doc;
            xml_node<> * root_node;
            std::ifstream theFile (file);
            std::vector<char> buffer((std::istreambuf_iterator<char>(theFile)), std::istreambuf_iterator<char>());
            buffer.push_back('\0');
            // Parse the buffer using the xml file parsing library into doc
            doc.parse<0>(&buffer[0]);
            // Find our root node
            root_node = doc.first_node("messages");
            for (xml_node<> * node = root_node->first_node("message"); node; node = node->next_sibling())
            {
                std::cout << node->first_attribute("id")->value() << std::endl;
                std::cout << "n = " << node-><< std::endl;

            }
            */

            break;
        }
        default:
            break;
        }



    } else {
        cout << "pas d'outil choisi.\n";
    }


    return 0;
}

void descriptionStation(std::string aShp){
    std::cout << "description du mileu pour les polygones d'un shp " << std::endl;
    cDicoApt dico(dirBD);
    //dico.summaryRasterFile();
    std::string header("");// pour TA du shp input
    std::string headerProcessing;// il y a le header qui concernent la table d'attribut du shp puis ceux-ci qui concernent les statistiques calculées

    GDALAllRegister();
    //ouverture du shp input
    const char *inputPath=aShp.c_str();
    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        std::cout << "shp chargé " << std::endl;


        // layer
        OGRLayer * lay = mDS->GetLayer(0);
        OGRFeature *poFeature=lay->GetFeature(0);

        std::map<std::string,int> statOrder;// en parallel computing il remplis le vecteur résultat dans un ordre aléatoire (push_back est effectué par le thread qui fini le plus rapidement
        // j'ai donc besoin d'une map pour savoir ordonner mes résultats
        int ind(0);

        int c(0);
        for (int i(0);i<poFeature->GetFieldCount();i++){
            std::string f=std::string(poFeature->GetFieldDefnRef(i)->GetNameRef());
            header+=f;
            if (i+1!=poFeature->GetFieldCount()){ header+=";";}
            statOrder.emplace(std::make_pair(f,c));
            c++;
        }
        std::vector<std::unique_ptr<rasterFiles>> aVRFs;
        std::map<int,std::vector<std::string>> aStat;



        // mode avec les raster de l'utilisateur
        if (dirMapDescr!="toto"){
            if (bf::is_directory(dirMapDescr)){
                std::vector<std::unique_ptr<rasterFiles>> aVRFs;
                std::cout << " ajout raster description de la station" << std::endl;

                for (bf::directory_iterator itr(dirMapDescr); itr!=bf::directory_iterator(); ++itr)
                {
                    //std::cout << itr->path().extension() << std::endl;
                    if (itr->path().extension()==".tif"){
                        //bf::path p;
                        //p.stem().string()
                        std::string var = itr->path().stem().string();
                        std::cout << " ajout " << var << "  " <<  " " << itr->path().string() << std::endl; // display filename only
                        std::unique_ptr<rasterFiles> r= std::make_unique<rasterFiles>(itr->path().string(),var);
                        headerProcessing+=";"+r->Code();
                        statOrder.emplace(std::make_pair(r->Code(),c));
                        aVRFs.push_back(std::move(r));
                        c++;
                    }
                }
                std::cout << "Total de " << aVRFs.size() << " raster ajouté " << std::endl;

                for (int id=0;id<lay->GetFeatureCount();id++){
                    poFeature = lay->GetFeature(id);
                    if (poFeature->GetFID() % 1000==0){std::cout << " process feature " << poFeature->GetFID() << std::endl;}
                    //if (poFeature->GetFID() == 2000){break;}
                    OGRGeometry * poGeom = poFeature->GetGeometryRef();
                    switch (poGeom->getGeometryType()){
                    case (wkbPolygon):{break;}
                    case wkbMultiPolygon:{break;}
                        // pour la carte générée pour analyse point, on ne dessine pas un polygone mais un cercle autour du point
                    case wkbPoint:
                    {
                        std::cout << " shp de point ; j'effectue un buffer de 18 m" << std::endl;
                        poGeom = poGeom->Buffer(25);
                        break;
                    }
                    default:
                        std::cout << "Geometrie " << poGeom->getGeometryName() << " non pris en charge " << std::endl;
                        break;
                    }
                    std::map<int,std::string> aStatOnPol;
                    // je commence par écrire dans le vecteur de résultat la valeur des champs de la table d'attribu, comme cela j'aurai mes identifiant
                    for (int i=0;i<poFeature->GetFieldCount();i++){
                        //aStatOnPol.push_back(poFeature->GetFieldAsString(i));
                        ind=statOrder.at(poFeature->GetFieldDefnRef(i)->GetNameRef());
                        aStatOnPol.emplace(std::make_pair(ind,poFeature->GetFieldAsString(i)));
                    }
                    OGRPoint * pt= getCentroid(poGeom->toPolygon());
                    double aX=pt->getX(),aY=pt->getY();
                    delete pt;
#pragma omp parallel num_threads(8) shared(aStatOnPol,aVRFs,aX,aY)
                    {
#pragma omp for
                        for (std::unique_ptr<rasterFiles> & r : aVRFs){
                            std::string aRes=std::to_string(r->getValueDouble(aX,aY));
#pragma omp critical
                            {
                                int ind2=statOrder.at(r->Code());
                                aStatOnPol.emplace(std::make_pair(ind2,aRes));
                            }

                        }
                    }
                    //maintenant on converti la map en vecteur de string
                    std::vector<std::string> vResOnPol;
                    for (auto kv : aStatOnPol){
                        vResOnPol.push_back(kv.second);
                    }
                    aStat.emplace(std::make_pair(poFeature->GetFID(),vResOnPol));
                }

            } else { std::cout << " vous avez spécifié un chemin d'accès pour des raster " << dirMapDescr << " mais celui-ci est introuvable " << std::endl;}
        } else{

            /*
             *
             *MODE NORMAL SANS chemin d'accès à des couches raster
             *
             */


            // selectionne les couches raster que je vais utiliser
            std::vector<std::string> aVCodes{"MNT","ZBIO","NT","NH","AE","SS","Topo","EP_FEE","SWC"};

            //std::vector<std::string> aVCodes{"MNT","ZBIO","NT","NH","AE","SS","Topo","EP_FEE","slope","MNH2019","MNH2014"};// ,"EP_CS","CS_A"

            /*
            std::vector<std::string> aVCodes{"ZBIO","NT","NH","Topo"};// plus toutes les essences pour avoir les Aptitude pour l'IPRFW
            for (std::string es : dico.getAllAcroEss()){
                aVCodes.push_back(es+"_FEE");
            }*/

            //std::vector<std::string> aVCodes{"MNT","ZBIO","NT","NH","AE","SS","Topo","BV_FEE","BP_FEE","slope"};
            //std::vector<std::string> aVCodes{"MNT","ZBIO","NT","NH","AE","SS","Topo"};
            //std::vector<std::string> aVCodes{"COMPO6","MNH2019","MNH2014","slope", "CS_A","CS3","CS8","HE_FEE","HE_CS"};
            std::vector<std::shared_ptr<layerBase>> aVLs;

            for (std::string aCode : aVCodes){
                if (dico.hasLayerBase(aCode)){
                    aVLs.push_back(dico.getLayerBase(aCode));
                } else { std::cout << "ne trouve pas la couche " << aCode << std::endl;}
            }


            if (climat){
                std::cout << " raster description du climat" << std::endl;
                // boucle sur les raster de données climatique et extraction de la valeur pour le centre de la tuile
                for (int y : vYears){
                    for (int m : vMonths){

                        for (std::string var : vVAR){
                            std::string code=var +"_"+std::to_string(y)+"_"+std::to_string(m);
                            std::string file(dirIRMMap +code +".tif");
                            std::unique_ptr<rasterFiles> r= std::make_unique<rasterFiles>(file,code);
                            aVRFs.push_back(std::move(r));
                        }
                    }
                }

                std::vector<std::string> var30{"ETP_30aire","P_30aire","R_30aire","Tmean_30aire","Tmax_30aire","Tmin_30aire"};
                for (std::string var : var30){
                    for (int m : vMonths){
                        std::string code=var +"_"+std::to_string(m);
                        std::string file(dirIRMMap +code +".tif");
                        std::unique_ptr<rasterFiles> r= std::make_unique<rasterFiles>(file,code);
                        aVRFs.push_back(std::move(r));
                    }
                    // moy trentenaire annuelle
                    std::string file(dirIRMMap +var +".tif");
                    std::unique_ptr<rasterFiles> r= std::make_unique<rasterFiles>(file,var);
                    aVRFs.push_back(std::move(r));
                }
            }

            /*
         * PREPARATION DES HEADERS
         *
         */

            bool isPt(0);// j'ai envie de faire les analyses non pas en mode surfacique mais en mode extract value to point

            // je pense que c'est assez long comme process (lecture CNSW)
            headerProcessing="";
            bool doCNSW=1;
            if (doCNSW){
                headerProcessing+="Texture;Texpct;Drainage;Dpct;Prof;Ppct";
                statOrder.emplace(std::make_pair("Texture",c));
                c++;
                statOrder.emplace(std::make_pair("Texpct",c));
                c++;
                statOrder.emplace(std::make_pair("Drainage",c));
                c++;
                statOrder.emplace(std::make_pair("Dpct",c));
                c++;
                statOrder.emplace(std::make_pair("Prof",c));
                c++;
                statOrder.emplace(std::make_pair("Ppct",c));
                c++;
            }


            for (std::shared_ptr<layerBase> & l : aVLs){
                if (!isPt){
                switch (l->getTypeVar()) {
                case TypeVar::Continu:{
                    switch (l->getCatLayer()) {
                    case TypeLayer::Peuplement:{
                        headerProcessing+=";"+l->Code()+"mean";
                        headerProcessing+=";"+l->Code()+"max";
                        headerProcessing+=";"+l->Code()+"sd";
                        statOrder.emplace(std::make_pair(l->Code()+"mean",c));
                        c++;
                        statOrder.emplace(std::make_pair(l->Code()+"max",c));
                        c++;
                        statOrder.emplace(std::make_pair(l->Code()+"sd",c));
                        c++;
                        break;
                    }
                    default:
                        headerProcessing+=";"+l->Code()+"mean";
                        statOrder.emplace(std::make_pair(l->Code()+"mean",c));
                        c++;
                    }
                    break;
                }
                case TypeVar::Classe:{
                    headerProcessing+=";"+l->Code()+"maj";
                    headerProcessing+=";"+l->Code()+"pct";
                    statOrder.emplace(std::make_pair(l->Code()+"maj",c));
                    c++;
                    statOrder.emplace(std::make_pair(l->Code()+"pct",c));
                    c++;
                    break;
                }
                default:
                    break;
                }
                }else {
                    headerProcessing+=";"+l->Code();
                    statOrder.emplace(std::make_pair(l->Code(),c));
                    c++;
                }
            }
            for (std::unique_ptr<rasterFiles> & r : aVRFs){
                headerProcessing+=";"+r->Code();
                statOrder.emplace(std::make_pair(r->Code(),c));
                c++;
            }

            /*
         * CALCUL
         *
         */
            //peut prendre 3 heures de calcul sur 100k tuiles, je devrai pe faire du checkpointing..
            //for (int cp(0) ; cp<lay->GetFeatureCount()/25000; cp++){
            //}

            for (int id=0;id<lay->GetFeatureCount();id++){
               // for (int id=0;id<100;id++){
                poFeature = lay->GetFeature(id);
                //std::cout << " process feature id " << id << std::endl;
                if (poFeature->GetFID() % 100==0){std::cout << " process feature " << poFeature->GetFID() << std::endl;}
                //if (poFeature->GetFID() == 2000){break;}
                OGRGeometry * poGeom = poFeature->GetGeometryRef();
                switch (poGeom->getGeometryType()){
                case (wkbPolygon):{break;}
                case wkbMultiPolygon:{break;}
                    // pour la carte générée pour analyse point, on ne dessine pas un polygone mais un cercle autour du point
                case wkbPoint:
                {
                    if (1){
                        //std::cout << " shp de point ; j'effectue un buffer de 18 m" << std::endl;
                        poGeom = poGeom->Buffer(25);
                    }
                    //std::cout << " point " << std::endl;
                    //isPt=1;
                    break;
                }
                default:
                    std::cout << "Geometrie " << poGeom->getGeometryName() << " non pris en charge " << std::endl;
                    break;
                }
                //poGeom->closeRings();
                //poGeom->flattenTo2D();
                //poGeom->MakeValid();
                std::map<int,std::string> aStatOnPol;

                // je commence par écrire dans le vecteur de résultat la valeur des champs de la table d'attribu, comme cela j'aurai mes identifiant
                for (int i=0;i<poFeature->GetFieldCount();i++){
                    //aStatOnPol.push_back(poFeature->GetFieldAsString(i));
                    ind=statOrder.at(poFeature->GetFieldDefnRef(i)->GetNameRef());
                    aStatOnPol.emplace(std::make_pair(ind,poFeature->GetFieldAsString(i)));
                }

                // cnsw
                if(doCNSW){
                    surfPedo statPedo(dico.mPedo,poGeom);
                    std::pair<std::string,double> t=statPedo.getMajTexture();
                    ind=statOrder.at("Texture");
                    aStatOnPol.emplace(std::make_pair(ind,t.first));
                    ind=statOrder.at("Texpct");
                    aStatOnPol.emplace(std::make_pair(ind,roundDouble(t.second,0)));
                    std::pair<std::string,double> d=statPedo.getMajDrainage();
                    ind=statOrder.at("Drainage");
                    aStatOnPol.emplace(std::make_pair(ind,d.first));
                    ind=statOrder.at("Dpct");
                    aStatOnPol.emplace(std::make_pair(ind,roundDouble(d.second,0)));
                    std::pair<std::string,double> p=statPedo.getMajProfCourt();
                    ind=statOrder.at("Prof");
                    aStatOnPol.emplace(std::make_pair(ind,p.first));
                    ind=statOrder.at("Ppct");
                    aStatOnPol.emplace(std::make_pair(ind,roundDouble(p.second,0)));
                }


                if (!isPt){
                    // les autres couches
#pragma omp parallel num_threads(8) shared(aStatOnPol,aVLs,statOrder)
                    {
#pragma omp for
                        for (std::shared_ptr<layerBase> & l : aVLs){
                            // analyse surfacique ; basic stat pour les var continue


                            switch (l->getTypeVar()) {
                            case TypeVar::Continu:{

                                basicStat stat=l->computeBasicStatOnPolyg(poGeom);

                                switch (l->getCatLayer()) {
                                case TypeLayer::Peuplement:{
#pragma omp critical
                                    {
                                        int ind2=statOrder.at(l->Code()+"mean");
                                        aStatOnPol.emplace(std::make_pair(ind2,stat.getMean()));
                                        ind2=statOrder.at(l->Code()+"max");
                                        aStatOnPol.emplace(std::make_pair(ind2,stat.getMax()));
                                        ind2=statOrder.at(l->Code()+"sd");
                                        aStatOnPol.emplace(std::make_pair(ind2,stat.getSd()));
                                    }
                                    break;
                                }
                                default:
#pragma omp critical
                                {
                                    //aStatOnPol.push_back(stat.getMean());
                                    int ind2=statOrder.at(l->Code()+"mean");
                                    aStatOnPol.emplace(std::make_pair(ind2,stat.getMean()));
                                }
                                }
                                break;
                            }
                            case TypeVar::Classe:{
                                std::pair<int,double> p= l->valMajoritaire(poGeom);
#pragma omp critical
                                {
                                    int ind2=statOrder.at(l->Code()+"maj");
                                    aStatOnPol.emplace(std::make_pair(ind2,std::to_string(p.first)));
                                    ind2=statOrder.at(l->Code()+"pct");
                                    aStatOnPol.emplace(std::make_pair(ind2,roundDouble(p.second,0)));
                                }
                                break;
                            }
                            default:
                                break;
                            }


                        }
                    }


                     // analyse ponctuelle, plus rapide.
                } else {
                    //std::cout << " analyse ponctuelle" << std::endl;

                    OGRPoint * pt= poGeom->toPoint();
                    double aX=pt->getX(),aY=pt->getY();
                    //delete pt;
#pragma omp parallel num_threads(8) shared(aStatOnPol, aVLs,aX,aY)
                    {
#pragma omp for
                        for (std::shared_ptr<layerBase> & l : aVLs){
                            std::string aRes=std::to_string(l->getValue(aX,aY));
#pragma omp critical
                            {
                                int ind2=statOrder.at(l->Code());
                                aStatOnPol.emplace(std::make_pair(ind2,aRes));
                            }

                        }
                    }


                }
                //std::cout << "done " << std::endl;


                /*
                 * CARTE CLIMAT
                 *
                 */
                if (climat){
                OGRPoint * pt= getCentroid(poGeom->toPolygon());
                double aX=pt->getX(),aY=pt->getY();
                delete pt;
#pragma omp parallel num_threads(8) shared(aStatOnPol,aVRFs,aX,aY)
                {
#pragma omp for
                    for (std::unique_ptr<rasterFiles> & r : aVRFs){
                        std::string aRes=std::to_string(r->getValueDouble(aX,aY));
#pragma omp critical
                        {
                            int ind2=statOrder.at(r->Code());
                            aStatOnPol.emplace(std::make_pair(ind2,aRes));
                        }

                    }
                }
                }

                //maintenant on converti la map en vecteur de string
                std::vector<std::string> vResOnPol;
                for (auto kv : aStatOnPol){
                    vResOnPol.push_back(kv.second);
                }

                aStat.emplace(std::make_pair(poFeature->GetFID(),vResOnPol));
            }

            std::cout << " finish to process features " << std::endl;
        }

        GDALClose(mDS);

        // sauve les stats dans un fichier texte
        std::string aFile(aShp.substr(0, aShp.size()-4)+"_stationDescriptor.csv");

        std::ofstream aOut;
        aOut.open(aFile,ios::out);
        //aOut << fixed << setprecision(2) << endl;

        aOut << header ;
        aOut << ";" << headerProcessing ;
        aOut << "\n" ;
        for (auto & kv : aStat){
            std::vector<std::string> v=kv.second;
            int c(0);
            for (std::string s : v){
                c++;
                aOut << s ;
                if (c!=v.size()) aOut << ";";
            }
            aOut << "\n";
        }
        aOut.close();
        std::cout << "done, file " << aFile << std::endl;

    } else {
        std::cout << "shp pas chargé, vérifier le nom de fichier entré avec arguement shp" << std::endl;
    }
}

void tuilage(std::string aShp){
    std::cout << "découpe les polygones d'un shp en tuile - unité de surface pour laquelle la station est +- homogène " << std::endl;
    int RectSize(25);
    GDALAllRegister();
    //ouverture du shp input
    const char *inputPath=aShp.c_str();
    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        std::cout << "shp chargé " << std::endl;
        // je devrais créer une grille sur toute l'emprise puis selectionner tout les polygones qui intersectent puis une découpe de ceux-ci si il sont pas entièrement dans le polygone puis sauver tout dans un shp
        // quid d'un alignement de ces tuiles avec le raster qui contient les informations que l'on va utiliser par après? pas trop important on va dire
        // quid de la taille des tuiles ; un multiple de la résolution de mes raster serait sans doute opportun, 30x30?
        OGRLayer * lay = mDS->GetLayer(0);
        std::vector<OGRGeometry*> vGeoms =squarebinPol(lay,RectSize);
        GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        std::cout << " create shp dataset " << std::endl;
        std::string aShpNameOut=aShp+"_tuile.shp";
        const char *out=aShpNameOut.c_str();
        GDALDataset * pShp=pShpDriver->Create(out, 0, 0, 0, GDT_Unknown, NULL );
        OGRLayer * layOut = pShp->CreateLayer("toto",nullptr,wkbPolygon,NULL);
        int c(0);
        for (OGRGeometry * geom : vGeoms){
            if (c % 100==0){std::cout << " process feature " << c << std::endl;}

            if ((OGR_G_Area(geom))>(0.7*((double)(RectSize*RectSize)))){
                OGRFeature * feat = new OGRFeature(layOut->GetLayerDefn());
                feat->SetGeometry(geom);
                layOut->CreateFeature(feat);
                delete feat;
            }
            c++;
        }
        GDALClose(pShp);
        GDALClose(mDS);
    } else {
        std::cout << "shp pas chargé, vérifier le nom de fichier entré avec arguement shp" << std::endl;
    }
}

std::vector<OGRPoint> squarebin(OGRLayer * lay, int rectSize){
    OGREnvelope ext;
    lay->GetExtent(&ext);
    std::vector<OGRPoint> aRes;
    //double rectSize(25.0);
    for (int i(0) ; i< ((ext.MaxX-ext.MinX)/(rectSize)); i++){
        for (int j(0) ; j< ((ext.MaxY-ext.MinY)/rectSize); j++){
            double x=ext.MinX+i*rectSize;
            double y=ext.MinY+j*rectSize;

            OGRPoint pt(x, y);
            lay->SetSpatialFilter(&pt);
            OGRFeature *poFeature;
            poFeature = lay->GetNextFeature();
            if  (poFeature!= NULL )
            {
                OGRFeature::DestroyFeature(poFeature);
                aRes.push_back(pt);
            }
            lay->SetSpatialFilter(NULL);
        }
    }
    std::cout << "got " << aRes.size() << " points in grid " << std::endl;
    return aRes;
}

std::vector<OGRGeometry*> squarebinPol(OGRLayer * lay, int rectSize){
    //double rectSize(25.0);
    std::vector<OGRPoint> pts=squarebin(lay,rectSize);
    std::vector<OGRGeometry *> aRes(pts.size());
    int c(0);
    for (OGRPoint p : pts){
        OGRLinearRing * ring = new OGRLinearRing();
        OGRPolygon * hex= new OGRPolygon();

        ring->addPoint(p.getX()-rectSize/2.0, p.getY()+rectSize/2.0);
        ring->addPoint(p.getX()+rectSize/2.0, p.getY()+rectSize/2.0);
        ring->addPoint(p.getX()+rectSize/2.0, p.getY()-rectSize/2.0);
        ring->addPoint(p.getX()-rectSize/2.0, p.getY()-rectSize/2.0);
        ring->closeRings();

        hex->addRingDirectly(ring);
        // confronter ce polygone au shp input pour voir si il faut le découper ou pas
        lay->SetSpatialFilter(&p);
        OGRFeature *poFeature;
        poFeature = lay->GetNextFeature();
        if  (poFeature!= NULL )
        {
            OGRGeometry* newGeo=hex;
            if (!hex->Within(poFeature->GetGeometryRef())){
                std::cout << "-" << std::endl;
                newGeo = hex->Intersection(poFeature->GetGeometryRef());
            }
            OGRFeature::DestroyFeature(poFeature);
            aRes[c]=newGeo;
        } else {
            std::cout << "problème!!!" << std::endl;
        }
        lay->SetSpatialFilter(NULL);


        c++;
    }
    //std::cout << "hexGeombin generate " << aRes.size() << " hexagones" << std::endl;
    return aRes;

}

void tuilageFromRaster(rasterFiles * rasterMask, int rectSize,double prop,double seuilRasterIn){
    // on va parcourir chaque vignette du raster et créer la géométrie si la proportion de 1 est >= prop
    // solution 1 plus propre.
    // mais alors on accepte des tuiles avec une taille étant un multiple de la résolution du raster...
    std::cout << "tuilage depuis un masque raster " << std::endl;

    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( rasterMask->getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << rasterMask->getPathTif() << std::endl;
    } else {
        // création du shp résultat
        GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
        std::cout << " create shp dataset " << std::endl;
        std::string aShpNameOut=rasterMask->getPathTif()+"_tuile.shp";
        const char *out=aShpNameOut.c_str();
        GDALDataset * pShp=pShpDriver->Create(out, 0, 0, 0, GDT_Unknown, NULL );
        OGRLayer * layOut = pShp->CreateLayer("toto",nullptr,wkbPolygon,NULL);
        int nbTuiles(0);

        GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

        double transform[6];
        mGDALDat->GetGeoTransform(transform);
        double xMin = transform[0];
        double yMax = transform[3];
        double pixelWidth = transform[1];
        double pixelHeight = -transform[5];
        double xMax = xMin+pixelWidth*mBand->GetXSize();
        double yMin = yMax-pixelHeight*mBand->GetYSize();

        // vérifier que la taille des vignettes est un multiple de la résolution
        if (rectSize % ((int) pixelWidth)!=0){std::cout << " Attention, la taille des tuiles " << rectSize << " n'est pas un multiple de la résolution du raster masque " << pixelWidth << ", je ne garanti rien\n\n\n" << std::endl;}
        //determine dimensions of the tile
        int xSize = round(rectSize/pixelWidth);
        int ySize = round(rectSize/pixelHeight);

        float *scanVignette;
        scanVignette = (float *) CPLMalloc( sizeof( float ) * xSize*ySize );
        // boucle sur chaque vignette
        for (int i(0) ; i< ((xMax-xMin)/(rectSize))-1; i++){
            for (int j(0) ; j< ((yMax-yMin)/rectSize)-1; j++){
                int nb(0);
                // lecture
                mBand->RasterIO( GF_Read, i*xSize, j*ySize, xSize, ySize, scanVignette, xSize,ySize, GDT_Float32, 0, 0 );
                for (int pix(0) ; pix<xSize*ySize;pix++){
                    if(scanVignette[pix]>=seuilRasterIn) {nb++;}
                }
                if (100.0*((double) nb)/((double)(xSize*ySize))>=prop){
                    nbTuiles++;
                    // création de la tuile
                    if (nbTuiles % 1000==0){std::cout << " création tuile " << nbTuiles << std::endl;}
                    OGRLinearRing * ring = new OGRLinearRing();
                    OGRPolygon * square= new OGRPolygon();
                    double x=xMin+i*xSize*pixelWidth;
                    double y=yMax-j*ySize*pixelHeight;
                    ring->addPoint(x, y);
                    ring->addPoint(x+xSize*pixelWidth, y);
                    ring->addPoint(x+xSize*pixelWidth, y-ySize*pixelWidth);
                    ring->addPoint(x, y-ySize*pixelWidth);
                    ring->closeRings();

                    square->addRingDirectly(ring);
                    OGRFeature * feat = new OGRFeature(layOut->GetLayerDefn());
                    feat->SetGeometry(square);
                    layOut->CreateFeature(feat);
                    delete feat;
                }
            }
        }
        CPLFree(scanVignette);
        mBand=NULL;
        GDALClose(pShp);
        GDALClose(mGDALDat);
    }

}


void echantillonTuiles(std::string aShp){
    std::cout << "echantillonTuiles " << std::endl;
    GDALAllRegister();
    //ouverture du shp input
    const char *inputPath=aShp.c_str();
    GDALDataset * mDS1 =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    GDALDriver *pShpDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    std::string aOut=aShp.substr(0, aShp.size()-4)+"_sub.shp";
    GDALDataset * mDS =pShpDriver->CreateCopy(aOut.c_str(),mDS1,FALSE, NULL,NULL, NULL );
    GDALClose(mDS1);
    GDALClose(mDS);
    mDS =  (GDALDataset*) GDALOpenEx( aOut.c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        OGRLayer * lay = mDS->GetLayer(0);
        //std::vector<unsigned int> indices(lay->GetFeatureCount());
        std::vector<int> indices;
        //std::iota(indices.begin(), indices.end(), 0);

        // boucle sur les features pour collecter les indices de tout ceux non scolyté
        OGRFeature *poFeature;
        int nbSamp(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            if (poFeature->GetFID() % 1000==0){std::cout << " process feature " << poFeature->GetFID() << std::endl;}
            double i =poFeature->GetFieldAsDouble("newSco")+poFeature->GetFieldAsDouble("oldSco");
            if (i<0.05){
                indices.push_back(poFeature->GetFID());
            } else {
                nbSamp++;
            }
        }
        // random mélange
        // obtain a time-based seed:
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine e(seed);
        std::shuffle(indices.begin(), indices.end(),e);
        std::cout << " indice size  " << indices.size() << " and nb non scolyté " << nbSamp << std::endl;
        for (int i(0);i<indices.size()-nbSamp;i++){
            //std::cout << " suppression de tuile non scolytée " << i << std::endl;
            //poFeature = lay->GetFeature();
            //OGRFeature::DestroyFeature(poFeature);
            lay->DeleteFeature(indices.at(i));
        }

        GDALClose(mDS);

    } else { std::cout << " shp " << aShp << " pas ouvert correctement " << std::endl;}
}

OGRPoint * getCentroid(OGRPolygon * hex){
    //std::cout << "getCentroid " << std::endl;
    OGRPoint ptTemp1;

    double x(1), y(1);
    int NumberOfVertices = hex->getExteriorRing()->getNumPoints();
    for ( int k = 0; k < NumberOfVertices; k++ )
    {
        hex->getExteriorRing()->getPoint(k,&ptTemp1);
        x+=ptTemp1.getX();
        y+=ptTemp1.getY();
    }
    x/=NumberOfVertices;
    y/=NumberOfVertices;
    return new OGRPoint(x,y);
}


void anaScolyteOnShp(rasterFiles * raster, std::string aShp){
    GDALAllRegister();
    //ouverture du shp input
    const char *inputPath=aShp.c_str();
    GDALDataset * mDS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_UPDATE, NULL, NULL, NULL );
    if( mDS != NULL )
    {
        // défini les nouveaux champs à ajouter à la table d'attribut - vérifie qu'il n'existe pas préhalablement
        OGRLayer * lay = mDS->GetLayer(0);
        if (lay->FindFieldIndex("newSco",0)==-1){
            OGRFieldDefn * oFLD= new OGRFieldDefn("newSco",  OFTReal);
            lay->CreateField(oFLD);
        }
        if (lay->FindFieldIndex("oldSco",0)==-1){
            OGRFieldDefn * oFLD= new OGRFieldDefn("oldSco",  OFTReal);
            lay->CreateField(oFLD);
        }
        if (lay->FindFieldIndex("sco",0)==-1){
            OGRFieldDefn * oFLD= new OGRFieldDefn("sco",  OFTReal);
            lay->CreateField(oFLD);
        }
        if (lay->FindFieldIndex("pix",0)==-1){
            OGRFieldDefn * oFLD2= new OGRFieldDefn("pix",  OFTReal);
            lay->CreateField(oFLD2);
        }

        // boucle sur les features
        OGRFeature *poFeature;
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {

            if (poFeature->GetFID() % 100==0){std::cout << " process feature " << poFeature->GetFID() << std::endl;}
            OGRGeometry * poGeom = poFeature->GetGeometryRef();
            switch (poGeom->getGeometryType()){
            case (wkbPolygon):{break;}
            case wkbMultiPolygon:{break;}
            case wkbPoint:
            {
                //std::cout << " shp de point ; j'effectue un buffer de 25 m" << std::endl;
                poGeom = poGeom->Buffer(25);
                break;
            }
            default:
                std::cout << "Geometrie " << poGeom->getGeometryName() << " non pris en charge " << std::endl;
                break;
            }

            basicStat bs=raster->computeBasicStatOnPolyg(poGeom);

            double freqNewSco = bs.getFreq(2)+bs.getFreq(22)+bs.getFreq(42);
            double freqOldSco = bs.getFreq(4)+bs.getFreq(21)+bs.getFreq(41)+bs.getFreq(43);
            double freqSco=freqOldSco+freqNewSco;
            /*if (freq>0){
            std::cout << " freq scolyte de " << freq << std::endl;
            }*/
            poFeature->SetField("oldSco",freqOldSco );
            poFeature->SetField("newSco",freqNewSco );
            poFeature->SetField("sco",freqSco );
            int nb =bs.getNbInt();
            poFeature->SetField("pix",nb);
            //poFeature->SetField(); This method has only an effect on the in-memory feature object. If this object comes from a layer and the modifications must be serialized back to the datasource, OGR_L_SetFeature()
            lay->SetFeature(poFeature);
        }

        GDALClose(mDS);

    } else { std::cout << " shp " << aShp << " pas ouvert correctement " << std::endl;}

}

