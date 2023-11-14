#include "caplicarteapt.h"
#include "capplicarteph.h"
#include "boost/program_options.hpp"
namespace po = boost::program_options;
using namespace std;

// écrire double dans cout avec 2 décimales
#include <iomanip>
// 2021 03 24 seb voudrai les matrices d'aptitude pour la RW, je vais utiliser forestimator plutôt que la BD FEE
void matriceApt(cDicoApt *dico, std::string aFile, int RN);

void replaceInDoc(std::string aFileIn, std::string aFind, double aReplace);
void replaceInDoc(std::string aFileIn, std::string aFind, std::string aReplace, bool chgCar=1);
void replaceInDoc(std::string aFileIn, std::string aFind, std::vector<std::string> aReplace);
// effectue 1 remplacement par élément du vecteur - la chaine de charactère à remplacer contient l'élément du vecteur en partant de 0
void replaceInDocVector(std::string aFileIn, std::string aFind, std::vector<std::string> aReplace);
void replaceFullLineInDoc(std::string aFileIn,std::string aReplace,int lineNumber);
std::vector<int> findLineInDoc(std::string aFileIn, std::string aFind);
std::string globToto("toto");
using namespace std;

std::string adirBD("/home/jo/app/Forestimator/carteApt/data/carteFEE_NTpH.db");
extern string columnPath;

//./carteApt --aptFEE 1 --pathBD "/home/jo/app/Forestimator/carteApt/data/aptitudeEssDB.db" --colPath Dir3

int main(int argc, char *argv[])
{
    po::options_description desc("options pour l'outil de calcul des cartes ");
    desc.add_options()
            ("help", "produce help message")
            ("carteNT", po::value<bool>(), "calcul de la carte des NT")
            ("cartepH", po::value<bool>(), "calcul de la carte des pH")
            ("carteNH", po::value<bool>(), "calcul de la carte des NH")
            ("aptFEE", po::value<bool>(), "calcul des cartes d'aptitude du FEE")
            ("aptCS", po::value<bool>(), "calcul des cartes d'aptitude du CS")
            ("MNH_TS", po::value<bool>(), "preparation de la série temporelle de MNH pour Forestimator")
            ("pathBD", po::value<std::string>(), "chemin d'accès à la BD carteFEE_NTpH.db ou à aptitudeEssDB.db")
            ("colPath", po::value<std::string>(), "nom de la colonne de fichierGIS et layerApt propre à la machine (chemin d'accès couche en local)")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        return 1;
    }
    bool carteNT(0),cartepH(0),carteFEE(0),carteCS(0),matApt(0),carteNH(0);
    if (vm.count("carteNT")) {carteNT=vm["carteNT"].as<bool>();}
    if (vm.count("cartepH")) {cartepH=vm["cartepH"].as<bool>();}
    if (vm.count("carteNH")) {carteNH=vm["carteNH"].as<bool>();}
    if (vm.count("aptFEE")) {carteFEE=vm["aptFEE"].as<bool>();}
    if (vm.count("aptCS")) {carteCS=vm["aptCS"].as<bool>();}
    if (vm.count("matApt")) {matApt=vm["matApt"].as<bool>();}
    if (vm.count("pathBD")) {adirBD=vm["pathBD"].as<std::string>();}
    if (vm.count("colPath")) {columnPath=vm["colPath"].as<std::string>();}



    // attention, les chemins d'accès pour les inputs et output ne sont pas les même pour cAppliCartepH que pour cApliCarteApt!! ne pas se gourer.
    if (carteNT | cartepH) {
        cAppliCartepH aAPH(adirBD,carteNT,cartepH);
    }

    if (carteNH) {
        calculNH(adirBD);
    }

    if (carteFEE | carteCS) {

        cDicoApt dico(adirBD);
        cApliCarteApt aACA(&dico);

        // je boucle les layersbase et non les essences car j'ai besoin du chemin d'accès au raster
        for (std::pair<std::string,std::shared_ptr<layerBase>>  kv : dico.VlayerBase()){

            std::string essCode(kv.second->EssCode()); //essCode =="AG"  | essCode =="AP" |essCode =="EK") |essCode =="PG" essCode =="PY" |essCode =="PZ"

            if (carteFEE &&(kv.second->getCatLayer()==TypeLayer::FEE)){
                aACA.carteAptFEE(dico.getEss(essCode),kv.second->getPathTif(),true);
            }

            if (carteCS &&(kv.second->getCatLayer()==TypeLayer::CS)){
                std::shared_ptr<cEss> ess = dico.getEss(essCode);
                aACA.carteAptCS(ess,kv.second->getPathTif(),true);
            }
        }
    }
    if (matApt){
        cDicoApt dico(adirBD);
        std::string aFileScribusTemp("/home/lisein/matAptRW2021/MatApt_Eco_RW.sla");
        for (int rn(1); rn <11;rn++){
            matriceApt(&dico, aFileScribusTemp, rn);
        }
    }

    if (vm.count("MNH_TS") && vm["MNH_TS"].as<bool>()){

        GDALAllRegister();
        std::string outPath;
        std::cout << "loop on file in " << adirBD << std::endl;
        /*for(auto & p : boost::filesystem::directory_iterator(adirBD)){
            std::string mnhPath= p.path().string();
            outPath=p.path().parent_path().parent_path().string() +"/MNHClip/";
            boost::filesystem::create_directory(outPath);
            // clip avec la limite de la RW
            //std::string aCommand ="gdalwarp -co 'COMPRESS=DEFLATE' -co 'TILED=YES' --config GDAL_CACHEMAX 512 -co NUM_THREADS=ALL_CPUS -co BIGTIFF=YES -cutline "+ columnPath +" -crop_to_cutline -dstnodata 32767 "+mnhPath+ " "+ outPath + p.path().filename().string();

            // en fait c'est tout de même très long, plus de 30 minutes par raster. Je devrais travailler autrement, avec un masque au format raster par exemple

            std::cout << aCommand << "\n";
            system(aCommand.c_str());
            aCommand ="gdaladdo -r average "+ outPath + p.path().filename().string()+ " 2 4 8 16 32 64";
            std::cout << aCommand << "\n";
            system(aCommand.c_str());
        }*/

        std::string aMask("/home/jo/Documents/Carto/MNH_TS/limitesWaGSD2.tif");
        GDALDataset * DSmask = (GDALDataset *) GDALOpen( aMask.c_str(), GA_ReadOnly );
        int x=DSmask->GetRasterBand(1)->GetXSize();
        float *scanlineMask = (float *) CPLMalloc( sizeof( float ) *x );

        for(auto & p : boost::filesystem::directory_iterator(adirBD)){
            std::string mnhPath= p.path().string();
            std::cout << "mnh " << mnhPath << std::endl;
            int y= std::stoi(mnhPath.substr(mnhPath.size()-8,mnhPath.size()-4));
            GDALDataset * DS = (GDALDataset *) GDALOpen( mnhPath.c_str(), GA_Update );
            DS->SetMetadataItem("Titre","Modèle Numérique de Hauteur de la canopée forestière");
            DS->SetMetadataItem("Année", std::to_string(y).c_str());
            DS->SetMetadataItem("Crédit","Gembloux Agro-Bio Tech");
            DS->SetMetadataItem("Unit","Les valeurs brutes du raster expriment la hauteur en centimètre. Un gain de 0.01 permet d'obtenir la hauteur en mètre (appliqué par défaut par QGis)");
            //DS->SetMetadataItem("scale_data","1");
            //DS->SetMetadataItem("scale_factor","0.01"); // je crois que ça sert à rien.
            DS->GetRasterBand(1)->SetScale(0.01);
            float *scanline = (float *) CPLMalloc( sizeof( float ) * x );
            for ( int row = 0; row < DSmask->GetRasterBand(1)->GetYSize(); row++ ){
                DSmask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanlineMask, x,1, GDT_Float32, 0, 0 );
                DS->GetRasterBand(1)->RasterIO( GF_Read, 0, row, x, 1, scanline, x,1, GDT_Float32, 0, 0 );
                // iterate on pixels in row
                for (int col = 0; col < x; col++)
                {
                     if(scanlineMask[ col ]==0){scanline[col]=32767;//std::cout << " no data found " << std::endl;
                     }
                }
                if (row%(DSmask->GetRasterBand(1)->GetYSize()/10)==0){std::cout <<  "..."<< std::endl;}

                DS->GetRasterBand(1)->RasterIO( GF_Write, 0, row, x, 1, scanline, x, 1,GDT_Float32, 0, 0 );
            }
            GDALClose(DS);
        }

        GDALClose(DSmask);

    }

    std::cout << "done" << std::endl;
    return 0;
}

void matriceApt(cDicoApt * dico, std::string aFile, int RN){

    // pour création des matrices d'aptitude
    std::map<std::string,std::shared_ptr<cEss>> VEss=dico->getAllEss();
    std::cout << "matrice pour Région " << dico->ZBIO(RN) << std::endl;
    std::string aOut(aFile+"_"+std::to_string(RN)+".sla");
    boost::filesystem::copy_file(aFile,aOut,boost::filesystem::copy_option::overwrite_if_exists);

    replaceInDoc(aOut,"RégionNat",dico->ZBIO(RN));

    for (auto kvNH : *dico->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
            std::string codeSribNH("");
            switch (codeNH) {
            case 6: codeSribNH="m4"; break;
            case 7: codeSribNH="m3"; break;
            case 8: codeSribNH="m2"; break;
            case 9: codeSribNH="m1"; break;
            case 10: codeSribNH="p0"; break;
            case 11: codeSribNH="p1"; break;
            case 12: codeSribNH="p2"; break;
            case 13: codeSribNH="p3"; break;
            case 14: codeSribNH="p4"; break;
            case 15: codeSribNH="p5"; break;
            case 17: codeSribNH="m1RHA"; break;
            case 18: codeSribNH="m2RHA"; break;
            case 19: codeSribNH="m3RHA"; break;
            }

            for (auto kvNT : *dico->NT()){
                int codeNT=kvNT.first;
                std::string codeSribNT("");
                switch (codeNT) {
                case 7: codeSribNT="A2"; break;
                case 8: codeSribNT="A1"; break;
                case 9: codeSribNT="M"; break;
                case 10: codeSribNT="C0"; break;
                case 11: codeSribNT="C1"; break;
                case 12: codeSribNT="C2"; break;
                }

                std::string codeNTNH=codeSribNT+codeSribNH;

                // creation de 3 maps listant les accronymes des essences et leurs aptitude
                std::map<std::string,std::vector<int>> O;
                std::map<std::string,std::vector<int>> T;
                std::map<std::string,std::vector<int>> TE;

                for (auto kv_ess : dico->getAllEss()){

                    std::shared_ptr<cEss> Ess=kv_ess.second;
                    std::string codeEss=kv_ess.first;

                    // if (codeEss=="AG" | codeEss=="AP" |  codeEss=="EP" ){
                    // on veut l'aptitude hydro-trophique, pas celle hierarchique Bioclim/HydroTroph.
                    int AptHT=Ess->getApt(codeNT,codeNH,RN,false);
                    int AptBIO=Ess->getApt(RN);
                    std::vector<int> apts;
                    apts.push_back(AptHT);
                    apts.push_back(AptBIO);

                    // apt la plus contraignante des deux. attention, cèdre a double apt TE/E en aptHT
                    // non ce n'est pas la bonne démarche. On garde l'aptitude HT même si elle est moins contraignante que apt Zbio, pour autant que Apt Zbio ne soit pas exclusion
                    //int Apt=Ess->getApt(codeNT,codeNH, RN,true);

                    //std::cout << " AptHT " << AptHT << "," << AptBIO << " AptBIO "  << " AptZbio " << Apt << std::endl;}

                    if( dico->AptNonContraignante(AptBIO)==1 | dico->AptNonContraignante(AptBIO)==2 | dico->AptNonContraignante(AptBIO)==3| dico->AptNonContraignante(AptBIO)==11){

                        //switch(dico->AptNonContraignante(Apt)){
                        switch(dico->AptNonContraignante(AptHT)){
                        case 1:
                            O.emplace(std::make_pair(codeEss,apts));
                            break;
                        case 2:
                            T.emplace(std::make_pair(codeEss,apts));
                            break;
                            // FN veux afficher l'essence en tolérance si indéterminé, avec 1 astérisque.
                        case 11:
                            T.emplace(std::make_pair(codeEss,apts));
                            break;
                        case 3:
                            TE.emplace(std::make_pair(codeEss,apts));
                            break;
                        }
                    }
                }

                for (int apt(1); apt<4;apt++){
                    //std::string toFind("<ITEXT CH=\""+codeNTNH+ "_"+ std::to_string(apt)+ "\"/>");
                    std::map<std::string,std::vector<int>> aM;
                    switch(dico->AptNonContraignante(apt)){
                    case 1:
                        aM=O;
                        break;
                    case 2:
                        aM=T;
                        break;
                    case 3:
                        aM=TE;
                        break;
                    }
                    std::string essCode(""), toReplace("");
                    std::string col("grey"),typo("");
                    int nbEss(0);
                    int fontsize(7);
                    if (aM.size()>16)fontsize=5;
                    if (aM.size()>25)fontsize=3;

                    typo="FONT=\"Arial Regular\" FONTSIZE=\""+std::to_string(fontsize)+"\" FEATURES=\"inherit\"";

                    for (auto kv : aM){

                        essCode=kv.first;
                        int AptHT(kv.second.at(0)), AptZBIO(kv.second.at(1));

                        // double apt O/E ; deux astérisques
                        if (AptHT==7 | AptZBIO==7){
                            essCode=essCode+"**";
                        } else if (dico->AptNonContraignante(AptHT)!=AptHT){
                            essCode=essCode+"*";
                            //std::cout << "double aptitude pour " << essCode << " , RN " << RN << std::endl;

                            // indéterminé ; on met en tolérance avec astérisque
                        } else if (AptHT==11 | AptZBIO==11){
                            essCode=essCode+"*";
                        }

                        switch(dico->AptNonContraignante(AptZBIO)){
                        case 1:
                            col="Optimum";
                            break;
                        case 2:
                            col="T1";
                            break;
                        case 11:
                            col="I";
                            break;
                        case 3:
                            col="TE";
                            break;
                        }
                        //if (nbEss!=0){toReplace=toReplace+"<ITEXT CH=\", \"/> \n";}
                        if (nbEss!=0){toReplace=toReplace+"<ITEXT CH=\" \"/> \n";}
                        toReplace=toReplace+"<ITEXT "+typo+ " FCOLOR=\""+col+"\" CH=\"" +essCode+ "\"/> \n";

                        nbEss++;
                    }

                    std::vector<int>  Line(findLineInDoc(aOut,codeNTNH));
                    if (Line.size()!=0){
                        int l(Line.at(0));
                        replaceFullLineInDoc(aOut,toReplace,l);
                    }

                    //replaceInDoc(aOut,toFind,toReplace,bool(0));
                }

            }
        }
    }

}

void replaceFullLineInDoc(std::string aFileIn,std::string aReplace,int lineNumber){

    int l(1);
    std::ifstream in(aFileIn);
    std::string aTmp(aFileIn+".tmp");
    std::ofstream out(aTmp);

    if (!in)
    {
        std::cout << "Could not open " << aFileIn << "\n";

    }

    if (!out)
    {
        std::cout << "Could not open " << aFileIn << "\n";

    }
    std::string line;

    while (getline(in, line))
    {
        if (l==lineNumber){
            line=aReplace;

        }

        out << line << "\n";
        l++;
    }
    out.close();
    in.close();
    std::remove(aFileIn.c_str());
    std::rename(aTmp.c_str(),aFileIn.c_str());
}

std::string findTxtInBaliseAtLine(std::string aFileIn, std::string aBalise, int lineNumber){
    int l(1);
    std::string aOut("");
    std::ifstream in(aFileIn);
    if (!in)
    {
        std::cout << "Could not open " << aFileIn << "\n";
    }
    std::string line;
    while (getline(in, line))
    {
        if ( l==lineNumber ){
            size_t pos = line.find(aBalise);
            if (pos != std::string::npos){
                std::string sub=line.substr(pos,pos+10);
                //std::cout << " sub string for balise " << aBalise << " is " << sub << std::endl;
                char *aBuffer = strdup((char*)sub.c_str());
                std::string aVal1Str = strtok(aBuffer,"\"");
                aOut = strtok( NULL, "\"" );
            }
            break;
        }
        l++;
    }
    in.close();
    return aOut;
}


void replaceInDoc(std::string aFileIn,std::string aFind,std::vector<std::string> aReplace){
    std::string aStr;
    for (auto & st : aReplace){
        //aStr=aStr+"\n\n"+st;
        if (st.size()!=0) aStr=aStr+st+globToto;
    }
    replaceInDoc(aFileIn,aFind,aStr);
}

void replaceInDoc(std::string aFileIn,std::string aFind,double aReplace){

    if((int)(10*aReplace)%10==0) {replaceInDoc(aFileIn,aFind,std::to_string((int)aReplace));} else {
        replaceInDoc(aFileIn,aFind,std::to_string(aReplace));}
}

void replaceInDocVector(std::string aFileIn,std::string aFind,std::vector<std::string> aReplace){
    std::string aStr;
    int i(0);
    for (auto & st : aReplace){
        std::string aFind2=aFind+std::to_string(i);
        //if (mDebug) std::cout << " find and replace in scribus doc " << aFind2 << std::endl;
        replaceInDoc(aFileIn,aFind2,st);
        i++;
    }
}


std::vector<int> findLineInDoc(std::string aFileIn,std::string aFind){

    int l(0);
    std::ifstream in(aFileIn);
    if (!in)
    {
        std::cout << "Could not open " << aFileIn << "\n";
    }
    std::vector<int> aLines;
    std::string line;

    while (getline(in, line))
    {
        l++;
        size_t pos = line.find(aFind);
        if (pos != std::string::npos){
            aLines.push_back(l);
        }
    }
    in.close();
    return aLines;
}


void replaceInDoc(std::string aFileIn, std::string aFind, std::string aReplace, bool chgCar){

    // il faut le faire dans le bon ordre, sinon remplace plusieur fois le meme charactère et le résultat resemble à rien
    if (chgCar){
        // saut de ligne : gestion très différente dans sribus 1.4 et 1.5. . 1.4; juste \n et c'est bon. 1.5; faut placer des balises.
        /*  if (sV15){
            boost::replace_all(aReplace, ";", globToto);
        } else {boost::replace_all(aReplace, ";", "\n");}*/
        boost::replace_all(aReplace, ";", globToto);
        boost::replace_all(aReplace, "&", " &amp;");
        boost::replace_all(aReplace, ">", " &gt;");
        boost::replace_all(aReplace, "<", " &lt;");
    }

    std::ifstream in(aFileIn);
    std::string aTmp(aFileIn+".tmp");
    std::ofstream out(aTmp);

    if (!in)
    {
        std::cout << "Could not open " << aFileIn << "\n";

    }

    if (!out)
    {
        std::cout << "Could not open " << aFileIn << "\n";

    }
    std::string line;
    size_t len = aFind.length();
    while (getline(in, line))
    {
        while (true)
        {
            size_t pos = line.find(aFind);
            if (pos != std::string::npos){
                line.replace(pos, len, aReplace);
            } else
                break;

        }
        out << line << "\n";
    }
    out.close();
    in.close();
    std::remove(aFileIn.c_str());
    std::rename(aTmp.c_str(),aFileIn.c_str());
}
