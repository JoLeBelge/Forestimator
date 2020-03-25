#ifndef CAPLICARTEAPT_H
#define CAPLICARTEAPT_H
#include "cdicoapt.h"
#include "gdal_priv.h"
#include "gdalwarper.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "cpl_string.h"
#include <iostream>
#include "boost/filesystem.hpp"
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"

using namespace std;

inline bool exists (const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
};

class cApliCarteApt
{
public:
    cApliCarteApt(cDicoApt * aDico);
    void carteAptFEE(cEss * aEss, std::string aOut, bool force=false);
    void carteAptCS(cEss * aEss, std::string aOut, bool force=false);
    void carteKKCS(cKKCS * aKK, std::string aOut, bool force=false);
    // gdal_translate pour compresser les résultats (à postériori, maintenant je le compresse au moment de la création des cartes.)
    void compressTif(std::string input);

    // le job de gdal_retile + la conversion des tuiles en png de couleur, veillez à ce que toutes les tuiles soient de 512x512 et suppression des tifs qui sont volumineux
    void createTile(std::string input, std::string outDir,TypeCarte aType=Apt, bool force=false);

    //conversion aptitude geotif to jpg pour utilisation dans openlayer (après tuilage avec gdal_retile)
    void toPNG(std::string input, std::string output,TypeCarte aType=Apt);
    void tiletoPNG(std::string aDir,TypeCarte aType=Apt);
    void cropIm(std::string input, std::string output, double topLeftX, double topLeftY,double width, double height);
    //clip avec l'extent d'un polygone
    void cropImWithPol(std::string inputRaster, std::string inputVector, std::string aOut);
    void toPol(std::string input, std::string output);
private:
private:
    cDicoApt * dico;
    // toute les couches ont la même résolution et le même extend.
    int x, y;
    // l'inconvénient c'est que je charge peut-être beaucoup de couche en mémoire vive. Enfin c'est pas encore trop
// FEE
    GDALDataset  * poDatNH;
    GDALDataset  * poDatNT;
    GDALDataset  * poDatZBIO;
    GDALDataset  * poDatTopo;
    GDALRasterBand * ZBIOBand;
    GDALRasterBand * NHBand;
    GDALRasterBand * NTBand;
    GDALRasterBand * TopoBand;
 // CS
    GDALDataset  * poDatCS1;
    GDALDataset  * poDatCS2;
    GDALDataset  * poDatCS10;
    GDALDataset  * poDatCS3;// Loraine
    GDALRasterBand * CS1Band;
    GDALRasterBand * CS2Band;
    GDALRasterBand * CS3Band;
    GDALRasterBand * CS10Band;
};

#endif // CAPLICARTEAPT_H
