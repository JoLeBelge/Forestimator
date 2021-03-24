#include "cdicoapt.h"
#include "caplicarteapt.h"

#include "cdicocarteph.h"
#include "capplicarteph.h"

// écrire double dans cout avec 2 décimales
#include <iomanip>

// 2021 03 24 seb voudrai les matrices d'aptitude pour la RW, je vais utiliser forestimator plutôt que la BD FEE
void matriceApt(std::map<std::string,std::shared_ptr<cEss>> aVEss,std::string aFile, int RN);


using namespace std;

int main(int argc, char *argv[])
{

    //cAppliCartepH aAPH=cAppliCartepH();
    // lecture de toutes les table dictionnaires
    cDicoApt dico(dirBD);
     cApliCarteApt aACA(&dico);
    std::map<std::string,std::shared_ptr<layerBase>> aMEss=dico.VlayerBase();
    std::map<std::string,cKKCS> aMKKs;

    // pour création des matrices d'aptitude
    std::map<std::string,std::shared_ptr<cEss>> VEss=dico.getAllEss();




    //std::map<std::string,cRasterInfo> aMRs;
    // creation des essences et de leur aptitudes FEE/CS
   /* for (auto & pair : *dico.codeEs2Nom()){
        // on ne crée plus l'essence car elle sont toutes créés par le dicitonnaire
        aMEss.emplace(std::make_pair(pair.first,cEss(pair.first,&dico)));
    }*/

  /*  for (auto & pair : *dico.codeKK2Nom()){
        aMKKs.emplace(std::make_pair(pair.first,cKKCS(pair.first,&dico)));
    }
*/
    // topo, nh, nt, ect
  //  for (auto & pair : *dico.RasterType()){
    //    aMRs.emplace(std::make_pair(pair.first,cRasterInfo(pair.first,&dico)));

        /*if (pair.first=="MNT"){
            cRasterInfo RI(pair.first,&dico);
            std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapThematiqueFEE.map");
            aACA.codeMapServer(RI.NomFileWithExt(),RI.Code(),RI.Nom(),aFileCodeMS,RI.getDicoVal(),RI.getDicoCol());
        }
        if (pair.first=="slope"){
            cRasterInfo RI(pair.first,&dico);
            std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapThematiqueFEE.map");
            aACA.codeMapServer(RI.NomFileWithExt(),RI.Code(),RI.Nom(),aFileCodeMS,RI.getDicoVal(),RI.getDicoCol());
        }

        if(pair.first.substr(0,5)=="COMPO" && pair.first.size()==6){
            cRasterInfo RI(pair.first,&dico);
            std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/compo.map");
            aACA.codeMapServer(RI.NomFileWithExt(),RI.Code(),RI.Nom(),aFileCodeMS,RI.getDicoVal(),RI.getDicoCol());

        }*/
    //}




    /*
    std::ofstream ofs ("./dico_colGrey.txt", std::ofstream::out | std::ofstream::trunc);
    // creation de ma palette de couleur grise pour MNT
    for (int i(0);i<255;i++){
        ofs << "g"<<i << "," <<i << "," << i << "," << i << std::endl;
    }
    ofs.close();

    ofs =std::ofstream("./dico_MNT.txt", std::ofstream::out | std::ofstream::trunc);
    ofs << std::fixed;
    ofs << std::setprecision(1);
    // creation du dictionnaire MNT
    for (int i(0);i<7001;i++){
        // une nouvelle couleur tout les 3 mètres
        std::string col("");
        if ((i % 30)==0){ col="g"+std::to_string(i/30);}
    ofs <<i << "," <<double(i/10.0)<< "," << col << std::endl;
    }

    ofs.close();
    */

/*
    cEss EP=aMEss.at("EP");
    for (auto & pair : EP.mEcoVal){
        for (auto & pair2 : pair.second){
        std::cout << " Zbio " << pair.first << "code nt nh " << pair2.first << " aptitude " << pair2.second << std::endl;
    }
    }
    //for(auto&& zbio : EP.mAptCS | boost::adaptors::map_values){
     for (auto & pair : EP.mAptCS){
         for (auto & pair2 : pair.second){
             std::cout << " Zbio " << pair.first << "code station " << pair2.first << " soit " << dico.station(pair.first,pair2.first) << ", aptitude " << pair2.second << std::endl;
         }
     }
     */



    //aACA.shptoGeoJSON("/home/lisein/Documents/carteApt/autres/epioux_parcellaire.shp","/home/lisein/Documents/carteApt/autres/test.geojson");


    //cEss HE=aMEss.at("HE");
    //std::cout << HE.printRisque() ;
    //aACA.carteAptFEE(&HE,HE.NomCarteAptFEE(),true);
    // creation des tuiles png pour utilisation dans openlayer
    //aACA.createTile(HE.NomCarteAptFEE(),HE.NomDirTuileAptFEE(),Apt,true);


if (1){


    /* std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapThematiqueFEE.map");
    for (auto & kv : aMRs){
        //aACA.createTile(kv.second.NomCarte(),kv.second.NomDirTuile(),kv.second.Type());
        cRasterInfo RI= kv.second;
         aACA.codeMapServer(RI.NomFileWithExt(),RI.Code(),RI.Nom(),aFileCodeMS,RI.getDicoVal(),RI.getDicoCol());
    }*/


    for (auto & kv : aMEss){

        // crée deux fois la carte, une fois pour layerbase CS et une fois pour lb FEE
        std::string essCode(kv.second->EssCode()); //essCode =="AG"  | essCode =="AP" |essCode =="EK") |essCode =="PG" essCode =="PY" |essCode =="PZ"
        if ((kv.second->getCatLayer()==TypeLayer::FEE) & (essCode =="PG" )){

        aACA.carteAptFEE(dico.getEss(kv.second->EssCode()),kv.second->getPathTif(),true);

        //aOut=dico.Files()->at("OUTDIR")+"aptitudeCS_"+kv.first+".tif";
        //aACA.carteAptCS(&kv.second,kv.second.NomCarteAptCS(),false);

         // compression
        //aACA.compressTif(kv.second.NomCarteAptCS());
        aACA.compressTif(kv.second->getPathTif());

        // creation des tuiles png pour utilisation dans openlayer
        //aACA.createTile(kv.second.NomCarteAptFEE(),kv.second.NomDirTuileAptFEE(),Apt,true);
       // aACA.createTile(kv.second.NomCarteAptCS(),kv.second.NomDirTuileAptCS(),Apt);

        //DicoVal=dico->code2AptFull();
        //DicoCol=dico->codeApt2col();
        //aACA.codeMapServer(kv.second.shortNomCarteAptFEE(),kv.second.NomMapServerLayer(),kv.second.NomMapServerLayerFull(),aFileCodeMS,Apt);
         /*if (exists(kv.second.NomCarteAptFEE())){
          * std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapCS.map");
            aACA.codeMapServer(kv.second.shortNomCarteAptCS(),kv.second.NomMapServerLayerCS(),kv.second.NomMapServerLayerFull(),aFileCodeMS,Apt);
         }*/
        }
        }

    //std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapKKCS.map");
    //for (auto & kv : aMKKs){
        //aACA.carteKKCS(&kv.second,kv.second.NomCarte(),false);
        //aACA.createTile(kv.second.NomCarte(),kv.second.NomDirTuile(),kv.second.Type());
        //aACA.compressTif(kv.second.NomCarte());
      //   cKKCS KK=kv.second;
         //aACA.codeMapServer(KK.shortNomCarte(),KK.NomMapServerLayer(),KK.NomMapServerLayerFull(),aFileCodeMS, KK.getDicoValPtr(),KK.getDicoCol());
    }





    // compression gdal des cartes input
    /*for (auto & kv : *dico.Files()){
        std::string path=kv.second;
        if (path.substr(path.size()-3,path.size())=="tif"){
            aACA.compressTif(path);
            }
        }
        */

    //aACA.tiletoPNG("/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/build-WebAptitude/test");

    // creation des tuiles pour le MNH photogrammétrique qui est en 2 mètres de résolution -- pas comme les autres cartes!
     //aACA.createTile("/home/lisein/Documents/carteApt/GIS/mnh_2019.tif","/home/lisein/Documents/carteApt/Forestimator/build-WebAptitude/Tuiles/MNH2019",MNH2019,true);



    return 0;
}


//void BDFEE::matriceAptitudeV2(std::string aFile, int RN){
void matriceApt(std::map<std::string,std::shared_ptr<cEss>> aVEss,std::string aFile, int RN){
    std::cout << "matrice pour Région " << mDico.RN(RN) << std::endl;
    std::string aOut(aFile+"_"+std::to_string(RN)+".sla");
    CopyFile(aFile,aOut);
    replaceInDoc(aOut,"RégionNat",mDico.RN(RN));

    for (auto kv_nt : * mDico.idNT2codeNT()){

        for (auto kv_nh : * mDico.idNH2codeNH()){

            std::string codeNTNH= kv_nt.second+kv_nh.second;
            // creation de 3 maps listant les accronymes des essences et leurs aptitude
            std::map<std::string,std::vector<int>> O;
            std::map<std::string,std::vector<int>> T;
            std::map<std::string,std::vector<int>> TE;

            for (auto ess : * mDico.GlobCodeEss()){
                // je crée l'essence
                Essence Ess(this,ess.first);

                std::string codeEss=mDico.codeEss4MatApt(ess.first);

                /* std::for_each(codeEss.begin(), codeEss.end(), [](char & c) {
                    c = ::tolower(c);
                });*/

                int AptHT=Ess.getApt(kv_nt.second, kv_nh.second, RN,false);
                int AptBIO=Ess.getApt(RN);
                std::vector<int> apts;
                apts.push_back(AptHT);
                apts.push_back(AptBIO);

                // apt la plus contraignante des deux. attention, cèdre a double apt TE/E en aptHT
                // non ce n'est pas la bonne démarche. On garde l'aptitude HT même si elle est moins contraignante que apt Zbio, pour autant que Apt Zbio ne soit pas exclusion
                int Apt=Ess.getApt(kv_nt.second, kv_nh.second, RN,true);

                //if (ess.second=="CD" && RN==1) {std::cout << " creation matrice apti , cèdre , ntxnh " << kv_nt.second << "," << kv_nh.second << ", RN " << RN << std::endl;

                //std::cout << " AptHT " << AptHT << "," << AptBIO << " AptBIO "  << " AptZbio " << Apt << std::endl;}

                switch(mDico.AptNonContraignante(Apt)){
                case 1:
                    O.emplace(std::make_pair(codeEss,apts));
                    break;
                case 2:
                    T.emplace(std::make_pair(codeEss,apts));
                    break;
                case 3:
                    TE.emplace(std::make_pair(codeEss,apts));
                    break;
                }
            }

            for (int apt(1); apt<4;apt++){
                //std::string toFind("<ITEXT CH=\""+codeNTNH+ "_"+ std::to_string(apt)+ "\"/>");
                std::map<std::string,std::vector<int>> aM;
                switch(mDico.AptNonContraignante(apt)){
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

                typo="FONT=\"Arial Regular\" FONTSIZE=\""+std::to_string(fontsize)+"\" FEATURES=\"inherit\"";

                for (auto kv : aM){

                    essCode=kv.first;
                    int AptHT(kv.second.at(0)), AptZBIO(kv.second.at(1));

                    // aptitude hydro-trophique ; typo. non on s'en fout car déjà visible car trié par aptitude

                    /*switch(mDico.AptNonContraignante(AptHT)){
                case 1:
                    typo="FONT=\"Arial Bold\" FEATURES=\"inherit allcaps\"";
                    break;
                case 2:
                    typo="FONT=\"Arial Regular\" FEATURES=\"inherit allcaps\"";
                    break;
                case 3:
                    typo="FONT=\"Arial Italic\" FEATURES=\"inherit\"";
                    break;
                }*/
                    if (mDico.AptNonContraignante(AptHT)!=AptHT){
                        essCode=essCode+"*";
                        //std::cout << "double aptitude pour " << essCode << " , RN " << RN << std::endl;
                    }

                    switch(mDico.AptNonContraignante(AptZBIO)){
                    case 1:
                        col="Optimum";
                        break;
                    case 2:
                        col="T1";
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
