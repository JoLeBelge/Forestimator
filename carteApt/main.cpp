#include "cdicoapt.h"
#include "caplicarteapt.h"

#include "cdicocarteph.h"
#include "capplicarteph.h"

// écrire double dans cout avec 2 décimales
#include <iomanip>


using namespace std;

int main(int argc, char *argv[])
{

    //cAppliCartepH aAPH=cAppliCartepH();
    // lecture de toutes les table dictionnaires
    cDicoApt dico(dirBD);
     cApliCarteApt aACA(&dico);
    std::map<std::string,cEss> aMEss;
    std::map<std::string,cKKCS> aMKKs;
    std::map<std::string,cRasterInfo> aMRs;
    // creation des essences et de leur aptitudes FEE/CS
    for (auto & pair : *dico.code2Nom()){
        aMEss.emplace(std::make_pair(pair.first,cEss(pair.first,&dico)));
    }

    for (auto & pair : *dico.codeKK2Nom()){
        aMKKs.emplace(std::make_pair(pair.first,cKKCS(pair.first,&dico)));
    }

    // topo, nh, nt, ect
    for (auto & pair : *dico.RasterType()){
        aMRs.emplace(std::make_pair(pair.first,cRasterInfo(pair.first,&dico)));

        if (pair.first=="MNT"){
            cRasterInfo RI(pair.first,&dico);
            std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapThematiqueFEE.map");
            aACA.codeMapServer(RI.NomFileWithExt(),RI.Code(),RI.Nom(),aFileCodeMS,RI.getDicoVal(),RI.getDicoCol());
        }
    }




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
        //std::string aOut=dico.Files()->at("OUTDIR")+"aptitudeFEE_"+kv.first+".tif";

        //aACA.carteAptFEE(&kv.second,kv.second.NomCarteAptFEE(),true);

        //aOut=dico.Files()->at("OUTDIR")+"aptitudeCS_"+kv.first+".tif";
        //aACA.carteAptCS(&kv.second,kv.second.NomCarteAptCS(),false);

         // compression
        //aACA.compressTif(kv.second.NomCarteAptCS());
        //aACA.compressTif(kv.second.NomCarteAptFEE());

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

    std::string aFileCodeMS("/home/lisein/Documents/carteApt/autres/mapserver/mapKKCS.map");
    for (auto & kv : aMKKs){
        //aACA.carteKKCS(&kv.second,kv.second.NomCarte(),false);
        //aACA.createTile(kv.second.NomCarte(),kv.second.NomDirTuile(),kv.second.Type());
        //aACA.compressTif(kv.second.NomCarte());
         cKKCS KK=kv.second;
         //aACA.codeMapServer(KK.shortNomCarte(),KK.NomMapServerLayer(),KK.NomMapServerLayerFull(),aFileCodeMS, KK.getDicoValPtr(),KK.getDicoCol());
    }



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
