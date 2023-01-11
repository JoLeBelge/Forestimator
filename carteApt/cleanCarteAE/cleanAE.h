

#include <cstring>
#include <algorithm>
#include "StdAfx.h"
#include "boost/filesystem.hpp"
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <filesystem>
#include <iostream>
#include "boost/program_options.hpp"

namespace fs=boost::filesystem;
namespace po = boost::program_options;

// copie des fonctions que j'ai développée pour nettoyer la carte d'apport en eau pour FEELU en
void cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois);
void cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois);
// fillHole
void fillHole(Im2D_U_INT1 * aIn, int Val2Clean, int ValConflict1, int ValCopain, int seuilVois, int aSz1);
void decompressRaster(std::string aIn, std::string aOut);
void compressTif(std::string aIn);
void checkCompression(std::string * aRaster);

std::string getNameTmp(std::string aName){
   return aName.substr(0,aName.size()-4)+"_tmp.tif";
}

void copyTifMTD(std::string aRasterIn, std::string aRasterOut){
    // copy projection et src dans gdal
    GDALDataset *pIn, *pOut;
    GDALDriver *pDriver;
    const char *pszFormat = "GTiff";
    pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    pOut = (GDALDataset*) GDALOpen(aRasterOut.c_str(), GA_Update);
    pIn = (GDALDataset*) GDALOpen(aRasterIn.c_str(), GA_ReadOnly);
    pOut->SetProjection( pIn->GetProjectionRef() );
    double tr[6];
    pIn->GetGeoTransform(tr);
    tr[1]=tr[1];
    tr[5]=tr[5];
    pOut->SetGeoTransform(tr);
    GDALClose(pIn);
    GDALClose(pOut);
}
