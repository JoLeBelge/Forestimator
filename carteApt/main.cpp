

#include "cdicoapt.h"
#include "caplicarteapt.h"


using namespace std;

int main(int argc, char *argv[])
{
    // lecture de toutes les table dictionnaires
    cDicoApt dico(dirBD);
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

    for (auto & pair : *dico.RasterType()){
        aMRs.emplace(std::make_pair(pair.first,cRasterInfo(pair.first,&dico)));
    }

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

    cApliCarteApt aACA(&dico);

    /*cEss HE=aMEss.at("HE");
    //std::cout << HE.printRisque() ;
    aACA.carteAptFEE(&HE,HE.NomCarteAptFEE());
    // creation des tuiles png pour utilisation dans openlayer
    aACA.createTile(HE.NomCarteAptFEE(),HE.NomDirTuileAptFEE(),Apt);*/

    for (auto & kv : aMRs){
        aACA.createTile(kv.second.NomCarte(),kv.second.NomDirTuile(),kv.second.Type());
    }

    for (auto & kv : aMEss){
        //std::string aOut=dico.Files()->at("OUTDIR")+"aptitudeFEE_"+kv.first+".tif";
        aACA.carteAptFEE(&kv.second,kv.second.NomCarteAptFEE(),false);
        //aOut=dico.Files()->at("OUTDIR")+"aptitudeCS_"+kv.first+".tif";
        aACA.carteAptCS(&kv.second,kv.second.NomCarteAptCS(),false);

         // compression
        //aACA.compressTif(kv.second.NomCarteAptCS());
        //aACA.compressTif(kv.second.NomCarteAptFEE());

        // creation des tuiles png pour utilisation dans openlayer
        aACA.createTile(kv.second.NomCarteAptFEE(),kv.second.NomDirTuileAptFEE(),Apt);
        aACA.createTile(kv.second.NomCarteAptCS(),kv.second.NomDirTuileAptCS(),Apt);
    }

    for (auto & kv : aMKKs){
        aACA.carteKKCS(&kv.second,kv.second.NomCarte(),false);
        aACA.createTile(kv.second.NomCarte(),kv.second.NomDirTuile(),kv.second.Type());
        //aACA.compressTif(kv.second.NomCarte());
    }

    // compression gdal des cartes input
    for (auto & kv : *dico.Files()){
        std::string path=kv.second;
        if (path.substr(path.size()-3,path.size())=="tif"){
            aACA.compressTif(path);
            }
        }

    // creation des tuiles pour les stations : ce n'est plus nécessaire, gérer par objet "RasterInfo"
/*
    for (auto & kv : *dico.ZBIO()){
        if (dico.Files()->find("CS"+to_string(kv.first))!=dico.Files()->end()){
            std::string aRasterPath(dico.Files()->at("CS"+to_string(kv.first)));
            std::string aDirOut(dico.Files()->at("OUTDIR2")+"CS"+to_string(kv.first));
            if (exists(aRasterPath)){
                aACA.createTile(aRasterPath,aDirOut,Station1);
            }
        }
    }
*/

    // clip des cartes apt sur la zone des épioux pour utilisation par Philippe
/*
    std::string aPathShp("/home/lisein/Documents/carteApt/autres/epioux_buf1k.shp");
    for (auto & kv : aMEss){
       // clip
        std::string aOut("/home/lisein/Documents/carteApt/autres/epiouxApt/"+kv.second.shortNomCarteAptFEE());
        std::string aOut2("/home/lisein/Documents/carteApt/autres/epiouxApt/"+kv.second.shortNomCarteAptCS());
        aACA.cropImWithPol(kv.second.NomCarteAptFEE(),aPathShp,aOut);
        aACA.cropImWithPol(kv.second.NomCarteAptCS(),aPathShp,aOut2);
        std::string aStyleFile=dico.Files()->at("styleApt");
        boost::filesystem::copy_file(aStyleFile,aOut.substr(0,aOut.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
        boost::filesystem::copy_file(aStyleFile,aOut2.substr(0,aOut2.size()-3)+"qml",boost::filesystem::copy_option::overwrite_if_exists);
    }



    cEss EP=aMEss.at("EP");
    std::string aOut=dico.Files()->at("OUTDIR")+"testCrop.tif";
    aACA.cropIm(EP.NomCarteAptFEE(),aOut,200000,90000,1000,1000);
    std::string aOut2=dico.Files()->at("OUTDIR")+"testCrop_topol.shp";
    aACA.toPol(aOut,aOut2);
    */
    //cEss EP=aMEss.at("EP");
    //std::string aOut=dico.Files()->at("OUTDIR")+"testJPG.png";


    //aACA.tiletoPNG("/home/lisein/Documents/carteApt/tutoWtANDOpenlayer/build-WebAptitude/test");




    return 0;
}
