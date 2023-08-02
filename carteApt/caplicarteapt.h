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
#include <fstream>
#import "date.h"

using namespace std;
using namespace date;

class color;

// creation du code mapserver pour une classe de couleur avec un label et une valeur de pixel associée
std::string MSClass(std::string label, std::string expression, color col);

class cApliCarteApt
{
public:
    cApliCarteApt(cDicoApt * aDico);
    ~cApliCarteApt();
    void carteAptFEE(std::shared_ptr<cEss> aEss, std::string aOut, bool force=false);
    void carteAptCS(std::shared_ptr<cEss> aEss, std::string aOut, bool force=false);

    void carteDeriveCS();

    // gdal_translate pour compresser les résultats (à postériori, maintenant je le compresse au moment de la création des cartes.)
    //void compressTif(std::string input);

    // creation du code de rendu de mapserver pour une couche donnée
    void codeMapServer(std::string inputData, string layerName, string layerFullName, std::string output, std::map<int, string> *DicoVal, std::map<int, color> DicoCol);

    //void cropIm(std::string input, std::string output, double topLeftX, double topLeftY,double width, double height);
    //clip avec l'extent d'un polygone
    //void cropImWithPol(std::string inputRaster, std::string inputVector, std::string aOut);
    void toPol(std::string input, std::string output);


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
    GDALDataset  * poDatCS1;// Ardenne
    GDALDataset  * poDatCS3;// Loraine
    GDALRasterBand * CS1Band;
    GDALRasterBand * CS3Band;
};

#endif // CAPLICARTEAPT_H
