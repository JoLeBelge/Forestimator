#include "cleanAE.h"


std::string pathAE("/home/jo/Documents/Carto/NH_RW2019/AE_W_202408.tif");

int main(int argc, char *argv[])
{
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("outils", po::value<int>()->required(), "choix de l'outil à utiliser")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    GDALAllRegister();
    int mode(vm["outils"].as<int>());

    switch (mode) {
    case 0:{
        std::cout << " nettoyage carte " << pathAE << std::endl;
        GDALDataset *pIn= (GDALDataset*) GDALOpen(pathAE.c_str(), GA_ReadOnly);
        bool test(0);
        //const char *comp = "DEFLATE";
        // comparaison de deux char * : pas d'overload pour ==, attention
        //if (strcmp(pIn->GetMetadataItem("COMPRESSION", "IMAGE_STRUCTURE"),comp)== 0){test=1;}
        const char *comp = "COMPRESSION=DEFLATE";
        if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){test=1;}
        GDALClose(pIn);

        if (test){
            std::cout << "compression détectée" << std::endl;
            if (!fs::exists(getNameTmp(pathAE))){
                // on décompresse tout ça
                std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ pathAE +" "+getNameTmp(pathAE)+" ");
                std::cout << aCommand << "\n";
                system(aCommand.c_str());

            }
             pathAE=getNameTmp(pathAE);
        }

        //lecture du raster
        std::cout << "charge image " << pathAE << std::endl;
        Im2D_U_INT1 * aIn=new Im2D_U_INT1(Im2D_U_INT1::FromFileStd(pathAE));

        //int Val2Clean(3),ValConflict1(10),ValCopain(10),seuilVois(5);
        // Val Conflict: les classe pour lesquelles ont ne veux pas de filtrage: ex filtrage de perte, ne peux pas toucher à apport variable
        // Val Copain; les classes qui sont assimilée à celles à nettoyer; ex app variable (Val to Clean) a pour copain les apports permanents: un pixels sans apport au milieu de Apport variable et Apport permanent va passer en apport variable

        std::cout << "clean apports permanent\n";

        int Val2Clean(2),ValConflict1(3),ValCopain(3),seuilVois(5);
        for (int iter(1); iter <3;iter++){
        // 49-25 = 24.
        seuilVois=14;
        fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(2));
        // 9pow 2= 81 - 5pow2=56
        //seuilVois=30;
        //fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(2));
        }


        Val2Clean=3;ValConflict1=1;ValCopain=666;
        for (int iter(1); iter <2;iter++){
        // 49-25 = 24.
        seuilVois=14;
        fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(2));
        // 9pow 2= 81 - 5pow2=56
        //seuilVois=30;
        //fillHole(aIn,Val2Clean,ValCopain,ValConflict1,seuilVois,int(2));
        }


        cleanVoisinage(aIn,1,666,int(7));
        cleanVoisinage(aIn,1,666,int(7));
        cleanVoisinage(aIn,1,666,int(7));

        // retirer les pixels 3 isolé dans du 2
        cleanVoisinage(aIn,2,666,int(7));

        // erode des 3 isolé en bordure des 2
        seuilVois=8;
        fillHole(aIn,1,2,2,seuilVois,int(1));




        // sauver resultat
        std::string aOut=pathAE.substr(0,pathAE.size()-4)+"_clean.tif";
        Tiff_Im::CreateFromIm(*aIn,aOut);
        copyTifMTD(pathAE,aOut);
        compressTif(aOut);

        break;
    }
    default:{
        std::cout << " mode outils incorrect " << std::endl;
    }
    }
    return 1;

}


int cn(3),cs(4),sco(2), newSco(22),oldSco(21),newCS(42), oldCS(41),newCSoldSco(43);


std::string path_otb("/home/lisein/OTB/OTB-7.3.0-Linux64/bin/");
std::string compr_otb="?&gdal:co:INTERLEAVE=BAND&gdal:co:TILED=YES&gdal:co:BIGTIFF=YES&gdal:co:COMPRESS=DEFLATE&gdal:co:ZLEVEL=9";

void compressTif(std::string aIn){
    std::string aCommand= std::string("gdalwarp -co 'COMPRESS=DEFLATE' -overwrite "+ aIn +" "+aIn.substr(0,aIn.size()-4)+"_co.tif ");
    //std::cout << aCommand << "\n";
    system(aCommand.c_str());
}


void checkCompression(std::string * aRaster){
    GDALDataset *pIn= (GDALDataset*) GDALOpen(aRaster->c_str(), GA_ReadOnly);
    bool test(0);
    const char *comp = "COMPRESSION=DEFLATE";
    if (strcmp(*pIn->GetMetadata("IMAGE_STRUCTURE"),comp)== 0){test=1;}
    GDALClose(pIn);
    if (test){
        std::cout << "compression détectée" << std::endl;
        std::string nameTmp=aRaster->substr(0,aRaster->size()-4)+"_tmp.tif";
        if (!fs::exists(nameTmp)){
            // on décompresse tout ça
            std::string aCommand= std::string("gdal_translate -co 'COMPRESS=NONE' "+ *aRaster +" "+nameTmp+" ");
            std::cout << aCommand << "\n";
            system(aCommand.c_str());

        }
         *aRaster=nameTmp;
    }
}

void cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}

void cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << " pour pixels isolé, seuil nombre de voisin " << seuilVois << ", remplace par : " << Val2Replace << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)==Val2Clean),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),aIn->in(4)==Val2Clean),1,IbinTmp->oclip());
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),1),ImNbVois.oclip());
    delete IbinTmp;
    // seuil voisin +1 car le pixels en question est compté dedans car de la mm catégorie mais n'est pas voisin de lui-même
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()<=seuilVois+1 && Im.in()==1),Val2Replace,aIn->oclip());
}

void fillHole(Im2D_U_INT1 * aIn,int Val2Clean, int ValCopain,int ValConflict1, int seuilVois,int aSz1){

    std::cout << "Nettoyage carte AE valeur " << Val2Clean << ", soutenu par valeur " << ValCopain <<", seuil nombre de voisin " << seuilVois << " Apport d'eau à ne pas modifier : " << ValConflict1 << std::endl;

    Im2D_U_INT1 Im(aIn->sz().x,aIn->sz().y,0);
    // pixels qu'on va peut-être changer de catégorie
    ELISE_COPY(select(aIn->all_pts(),aIn->in(0)!=Val2Clean && aIn->in(0)!=ValConflict1),1,Im.oclip());
    //statistique sur le nombre de pixel voisin qui sont de la classe d'AE à nettoyer
    Im2D_U_INT1 ImNbVois(aIn->sz().x,aIn->sz().y,0);
    Im2D_BIN * IbinTmp=new Im2D_BIN(aIn->sz().x,aIn->sz().y,0);
    ELISE_COPY(select(aIn->all_pts(),(aIn->in(4)==Val2Clean)| (aIn->in(4)==ValCopain)),1,IbinTmp->oclip());

    // 49 pixels de voisinage  si aSz1 est de 3 (3+1+3) pow 2
    // si sz=2, 25 pixels
    // Si sz=1; 9
    ELISE_COPY(aIn->all_pts(),rect_som(IbinTmp->in(0),aSz1),ImNbVois.oclip());
    delete IbinTmp;
    ELISE_COPY(select(aIn->all_pts(),ImNbVois.in()>=seuilVois && Im.in()==1),Val2Clean,aIn->oclip());
}



