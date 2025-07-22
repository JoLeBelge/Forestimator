#ifndef CFEELU_H
#define CFEELU_H
#include "StdAfx.h"
#include "cfeatheringandmosaicking.h"


#include <unistd.h>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

std::map<int,std::vector<std::string>> parseCSV(std::string aFileIn, char aDelim);
std::string NameTFW(std::string aOrtName);



Fonc_Num dilateEcoul(Fonc_Num f,Fonc_Num ChDist,int aDist);


template <class Type,class TyBase>
void slope_degrees
     (
          Im2D<Type,TyBase> Iout,
          Im2D<Type,TyBase> Iin,
          INT r
     );

class  c_Appli_FEELU;

class  c_Appli_FEELU
{
public:
    c_Appli_FEELU(int argc,char ** argv);

private:
    std::string mDir;
    std::string mTFWin, mFileEcoul;
    std::string mNameMNT,mNameDelatHOut, mFullDir,mNameLabel,mNameHydro,mNameFlowAccIn,mTmpDir,mNameDistOut,mNameAE,mNameEcoulement;
    std::string mNameMNTrel2,mNameDist2;
    cInterfChantierNameManipulateur * mICNM;

    std::string mNameInondation;

    std::string mNameTPI1,mNameTPI2,mNameTPI3,mNameTPI4,mNameSlope;

    std::string mNameAEcombine,mNameAEcleaned;
    // trop d'image en RAM, je tente d'en laisser en Tiff_im
    Tiff_Im * mLabel;
    Im2D_U_INT1 mDistChanf, mEcoul;
    Pt2di sz;
    Tiff_Im * mMNT;
    Im2D_REAL4 mMNTrel;
    Im2D_REAL4 mFA;
    Im2D_U_INT2 mSurf;
    Liste_Pts_INT2 * lpHydro;

    Im2D_U_INT2 mHydro;
    //Tiff_Im * mHydro;
    // FA dilaté autour du réseau hydro
    std::string mNameFlowAccOut;

    bool mForce;
    int mMode;

    std::map<int, Im2D_REAL4>   mIm2Ds;
    std::map<int, Im2D_INT2>    mChamferDist;
    bool mDebug;
    int mDist, seuilDist;

    void loadOrCompute(std::string aName,Im2D_REAL4 * aMNT, Im2D_INT2 *aOut, int aFactR, int aDist, std::string aType);

    void ChamferNoBorder(Im2D<U_INT1,INT> * i2d);
    void computeDistMap1by1(Im2D_U_INT1 *Ibin, Im2D_U_INT1 * aOut);
    void add2DistMap(Liste_Pts_INT2 * lp, Pt2di pmin,Pt2di pmax,Im2D_U_INT1 * aOut, int DistMax);
    void computeMNTrel();
    void computeFA();
    // lit le FAdilaté et exporte les valeurs pour analyse dans R
    void readVal4EachRiver(string aOut);
    void updateMNTrel4SurfaceEau(Im2D_U_INT1 * aDistSurf);
    void computeEcoulement();

    void surfaceCurvatureIndex();

    void slope(Im2D_REAL4 * aMNT, Im2D_INT2 *aOut, int aFactR, int aDist);

    void clean(Im2D_U_INT1 * aIn);
    void cleanVoisinage(Im2D_U_INT1 * aIn,int Val2Clean, int ValConflict1, int seuilVois);
    void cleanIsolatedPix(Im2D_U_INT1 * aIn,int Val2Clean, int Val2Replace, int seuilVois);

    // fillHole
    void fillHole(Im2D_U_INT1 * aIn, int Val2Clean, int ValConflict1, int ValCopain,int seuilVois, int aSz1, int aSz2);

    // les stat de focale sont calculés sur le MNT resampled, puis remis à résol=1 pour soustraire à MNT initial.
    void TPIbyResampling(Im2D_REAL4 * aMNT, Im2D_INT2 *aOut, int aFactR, int aDist, string aType);
    int mParam;


    template <class T,class TB> void _dilate(Im2D<T,TB> * aIm2Dilate,Im2D_U_INT1 & aImDist,double aND,bool aMoy=0);


};


#endif // CFEELU_H
