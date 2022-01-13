#ifndef LAYERBASE_H
#define LAYERBASE_H

#include "cdicoapt.h"
#include <boost/algorithm/string/replace.hpp>
#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <numeric>
#include "color.h"

std::string getAbbreviation(std::string str);

GDALDataset * rasterizeGeom(OGRGeometry *poGeom, GDALDataset * aGDALDat);

// renvoie une liste de polgone qui sont des hexagones
std::vector<OGRPolygon*> hexGeombin(GDALDataset *mask);
// renvoie une grille de point, centre d'hexagone
std::vector<OGRPoint> hexbin(GDALDataset * mask);


double predHdom(std::vector<double> aVHs);
double getQ95(std::vector<double> aVHs);
// dans Forestimator, un gros problème, c'est le fait que j'ai ma classe layer qui soiet strictement liée à Wt et à Forestimator (via grouplayer)
// J'aurai du avoir une classe mère qui soie indépendante de Wt et me permette de faire des stats sur des cartes, exactement comme j'en ai besoin maintenant pour stationDescriptor
// Du coup toute mes classe qui terminent par Base sont des classes SANS Wt qui sont intégrée dans le dictionnaire et qui sont donc utilisée pour les traitements API et autre du genre

class LayerMTD; // classe tout à fait à par qui sert uniquement à générer la rubrique de documentation de forestimator pour cette couche
class WMSinfo;
class basicStat;

class cEss; // avec les aptitudes de l'essence
class cKKCS; // ce qui caractérise les stations ; potentiel sylvicole, facteur écologique, risques

class color;

class rasterFiles; // une classe dédiée uniquemnent à l'export et au clip des couches.
// rasterinfo hérite de rasterFile
class layerBase;

class layerStat;

// ça c'était utilisé pour le calcul des carte apt, des tuiles et du code mapserveur
enum TypeCarte {Apt, Potentiel, Station1, Habitats,NH,NT,Topo,AE,SS,ZBIO,CSArdenne,CSLorraine,MNH,Composition,MNT16b};


enum class TypeVar {Classe,
                    Continu
                   };

enum class TypeLayer {
    KK // les cartes dérivées des CS
    ,Station // lié à la description de la station, approche FEE ; NH NT ZBIO, mais aussi les stations du CS
    ,Externe // toutes les cartes qui ne sont pas en local ; carte IGN pour commencer
    ,FEE // aptitude
    ,CS // aptitude
    ,Peuplement // description du peuplement en place
};

enum class TypeWMS {WMS,
                    ArcGisRest
                   };

// forward dec
class cDicoApt;



inline bool exists (const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

TypeCarte str2TypeCarte(const std::string& str);
TypeVar str2TypeVar(const std::string& str);
TypeLayer str2TypeLayer(const std::string& str);
TypeWMS str2TypeWMS(const std::string& str);

class basicStat{
public:

    basicStat():min(0),max(0),mean(0),stdev(0),nb(0){}
    basicStat(std::map<double,int> aMapValandFrequ, double na=0.0);
    basicStat(std::vector<double> v);
    std::string getMin(){return roundDouble(min);}
    std::string getMax(){return roundDouble(max);}
    std::string getMean(){return roundDouble(mean);}
    std::string getNb(){return std::to_string(nb);}
    int getNbInt(){return nb;}
    std::string getSd(){return roundDouble(stdev);}
    std::string getCV(){if (mean!=0) {return roundDouble(100.0*stdev/mean)+"%";} else { return "-1";};}

    double getFreq(double aVal){
        double aRes(0);
        if (mValFreq.find(aVal)!=mValFreq.end()){
            //std::cout << " freq pour val " << aVal << " est de " << mValFreq.at(aVal) << " pixel sur un total de " << nb << std::endl;
            aRes=mValFreq.at(aVal)/(double (nb));
        }
        return aRes;
    }

    double getMeanDbl(){return mean;}
   private:
    double min,max,mean,stdev;
    int nb;
    std::map<double,int> mValFreq;// utile quand je veux des stats basiques sur un raster de variable discontinu
};


class statCellule{
public:
    statCellule(std::vector<double> *aVHs, int aSurf,bool computeDendro=0);
    //void computeHdom(){mHdom=k1hdom*mQ95+k2hdom*pow(mQ95,2);}

    // OLD OLD maintenant c'est une approche pixel
    /*
    void computeGha(){ if (mHdom!=0.0){mGha=k1gha*mVHA/mHdom;} else {mGha=0.0;}
                     }
    void computeNha(){mNha=40000.0*M_PI*mGha/pow(mCmoy,2);
                     // peut me renvoyer inf par moment
                      if (isinf(mNha) | isnan(mNha)){mNha=0.0;}
                     }
    void computeCmoy(){mCmoy=(k1cmoy*(mHdom-1.3)+k2cmoy*pow((mHdom-1.3),2))*pow(mMean/mQ95,k3cmoy);}*/

    //SI(Hpixi <= K2; 0; K1*(Hpixi-K2)^K3)



    //void computeHdom(){

    void printDetail();

    double mVHA, mHdom, mGha, mCmoy, mNha;
     int mSurf;
private:
    double mMean,mQ95;

};


class statHdomBase {
public:
    statHdomBase(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom,bool computeStat=1);
    ~statHdomBase(){
       for (OGRPolygon * pol: mVaddPol) OGRGeometryFactory::destroyGeometry(pol);
       mVaddPol.clear();
       mStat.clear();
       mDistFrequ.clear();
    }
    std::shared_ptr<layerBase> Lay(){return mLay;}
    bool deserveChart();
    cDicoApt * Dico();

    void predictHdomHex();
    void predictDendro(bool onlyHdomStat=1);

    //std::map<std::string, double> computeDistrH();
    std::vector<std::pair<std::string,double>> computeDistrH();

    basicStat bshdom();
    basicStat bsDendro(std::string aVar="hdom");

protected:
    std::shared_ptr<layerBase> mLay;
    //std::vector<double> mStat; // un vecteur ; une valeur par cellule d'un are.
    std::vector<std::unique_ptr<statCellule>> mStat;
    //std::vector<std::string,double>
    std::vector<std::pair<std::string,double>>mDistFrequ;// pair avec range de valeur (genre 3-9) et proportion de la distribution
    OGRGeometry * mGeom;
    // geometrie supplémentaire à afficher sur l'image statique
    std::vector<OGRPolygon *> mVaddPol;
    int mNbOccurence;
};

// modèle reçu de jérome le 9/06/2021
// reçu update du modèle par Adrien. 1) seul le MNH 2018 est suffisament correct (selon philippe). 2) la résolution des couches d'entrainement du modèle est de 5m ET c'est un MNH percentile 95 (on applique donc le percentile à deux reprises) 3) des modèles "pixels" sont ajustés, plus facile d'utilisation que les modèles "placettes (=objectif premier de la classe cellulle)
//extern double k1hdom, k2hdom, k1vha,k2vha,k3vha, k1cmoy,k2cmoy;//,k3cmoyk1gha,;

class statDendroBase : public statHdomBase{
public:
    statDendroBase(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom,bool api=0);
    void predictDendroPix();
    bool deserveChart();

    std::string getNha();
    std::string getVha();
    std::string getGha();
    std::string getHdom();
    std::string getCmoy();
};


class WMSinfo
{
    public:
    std::string WMSLayerName()const{return mWMSLayerName;}
    std::string WMSURL()const{return mUrl;}
    TypeWMS getTypeWMS(){return mTypeWMS;}

    WMSinfo():mUrl(""),mWMSLayerName("toto"){}
    WMSinfo(std::string url,std::string layer, std::string aTypeGeoservice,std::string attribution):mUrl(url),mWMSLayerName(layer),mWMSattribution(attribution){
        mTypeWMS=str2TypeWMS(aTypeGeoservice);
    }
    std::string mUrl, mWMSLayerName, mWMSattribution;

    TypeWMS mTypeWMS;
};


// pour afficher en html ou pdf les informations relatives aux couches, aux stations des cs, au méthodologies (genre calcul de hdom)
class LayerMTD{
public:
    LayerMTD(){}
    void setCode(std::string aCode){mCode=aCode;}
    void setNom(std::string aNom){mNom=aNom;}
    void setLabel(std::string a){mShortName=a;}
    void setProjet(std::string aProj){mProjet=aProj;}
    void setDescr(std::string aDesc){mDescr=aDesc;}
    void setVersion(std::string aV){mVersion=aV;}
    void addRef(std::string aVRef){mVRefs.push_back(aVRef);}
    void setCopyR(std::string a){mCopyR=a;}
     std::string code(){return mCode;}
    std::string Nom(){return mNom;}
    std::string Label(){return mShortName;}
    std::string Descr(){return mDescr;}
    std::string Vers(){return mVersion;}
    std::string Projet(){return mProjet;}
    std::string CopyR(){return mCopyR;}
    std::vector<std::string> VRefs(){return mVRefs;}
private:
    std::vector<std::string> mVRefs;
    std::string mProjet, mDescr,mVersion, mNom,mCopyR,mShortName,mCode;
};

class rasterFiles{
public:
    rasterFiles(std::string aPathTif,std::string aCode);
    std::string Code() const{return mCode;}
    void checkForQml();
    // retourne le chemin d'accès complêt
    std::string getPathTif() const{return mPathRaster;}
    std::string symbology() const{return mPathQml;}
    bool hasSymbology() const{return mPathQml!="";}
    rasterFiles getRasterfile();
    bool rasterExist(){return exists(mPathRaster);}

    // méthode GDAL
    int getValue(double x, double y);
    double getValueDouble(double x, double y);
    // rasterfile, pas de dictionnaire
    basicStat computeBasicStatOnPolyg(OGRGeometry * poGeom);
    GDALDataset * rasterizeGeom(OGRGeometry *poGeom);
protected:
    std::string mPathRaster, mPathQml, mCode;
};

class layerBase : public rasterFiles, public WMSinfo, public std::enable_shared_from_this<layerBase>
{
public:
    layerBase(std::string aCode,cDicoApt * aDico);
    layerBase(std::shared_ptr<layerBase> aLB);

    std::string NomFile(); // nom du fichier tiff sans l'extension
    std::string NomFileWithExt();

    std::string Nom(){return mNom;}
    std::string NomCourt(){return mNomCourt;}
    std::string getLegendLabel(bool escapeChar=true) const;
    std::string getShortLabel() const {return mNomCourt;}

    TypeCarte TypeCart(){return mTypeCarte;}
    std::map<int, std::string> getDicoVal(){return mDicoVal;}
    std::map<int, color>  getDicoCol(){return mDicoCol;}

    TypeVar getTypeVar() const{return mTypeVar;}
    TypeLayer getCatLayer() const{return mType;}
    bool Expert() const{return mExpert;}
    double Gain() const{return mGain;}
    void catSummary(){std::cout << "layerbase ; Code " << mCode << " , Nom " << mNom << ", raser " << mPathRaster << ", dictionnaire valeurs de " << mDicoVal.size() << " elements " << std::endl;}

    std::string EssCode() const{std::string aRes="";
                                   if ((mType==TypeLayer::CS) |(mType==TypeLayer::FEE)){aRes=mCode.substr(0,2);}
                                   return aRes;}
    std::string getCatLayerStr() const{std::string aRes("");
      if (mType==TypeLayer::KK){aRes="KK";}
      if (mType==TypeLayer::Station){aRes="Station";}
      if (mType==TypeLayer::Externe){aRes="Externe";}
      if (mType==TypeLayer::FEE){aRes="FEE";}
      if (mType==TypeLayer::CS){aRes="CS";}
        return aRes;}

    // stat sur un polygone ; deux retour possible, une map avec clé = valeur raster, une map avec clé = signification string
    // clé ; signification du code raster. Val ; nombre d'occurence
    std::map<std::string,int> computeStat1(OGRGeometry * poGeom);
    // clé ; val raster. val ; pct. Comment gerer les no data? code int -1?
    std::map<int,double> computeStat2(OGRGeometry * poGeom);
    // retourne la valeur majoritaire sur le polygone ainsi que son prct en surface
    std::pair<int,double> valMajoritaire(OGRGeometry * poGeom);

     std::string summaryStat(OGRGeometry * poGeom);

    // ça c'est pour les couches variables continues
    basicStat computeBasicStatOnPolyg(OGRGeometry * poGeom);

    bool hasColor(int aCode) const{
        bool aRes(0);
        if (mDicoCol.find(aCode)!=mDicoCol.end()){aRes=true;}
        return aRes;
    }
    color getColor(int aCode) const{
        color aRes(0,0,0);
        if (mDicoCol.find(aCode)!=mDicoCol.end()){
            aRes=mDicoCol.at(aCode);}
        return aRes;
    }
    color getColor(std::string aStrCode) const;
    cDicoApt * Dico(){return mDico;}

    // convertir le wms de la couche au format image en local
    bool wms2jpg(OGREnvelope *extent, int aSx, int aSy, std::string aOut) const;

    std::string getValLabel(int aVal){
        std::string aRes("");
        if (mDicoVal.find(aVal)!=mDicoVal.end()){aRes=mDicoVal.at(aVal);}
        return aRes;
    }

protected:
    TypeCarte mTypeCarte;
    TypeVar mTypeVar; // var continue ou discontinue, pour le calcul de statistique
    TypeLayer mType;
    cDicoApt * mDico;
    std::string mNom,mNomCourt;
    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> mDicoVal;

    // le dictionnaire des valeurs raster vers la couleur.
    std::map<int, color> mDicoCol;
    // pour distinguer les cartes qui sont accèssible à tous ou pas
    bool mExpert;
    double mGain;
};

class layerStat
{
public:
    layerStat(std::shared_ptr<layerBase> aLay,std::map<std::string,int> aStat);

    void simplifieStat();
    std::string summaryStat();
    int getO(bool mergeOT=false);// proportion en optimum

    //int getFieldVal(bool mergeOT=false);
    //std::string getFieldValStr();

    std::shared_ptr<layerBase> Lay(){return mLay;}

    std::map<std::string, int> StatSimple(){return mStatSimple;}
    std::map<std::string, int> Stat(){return mStat;}
    TypeVar mTypeVar; // pour distinguer le type de variable, continue (MNH) ou classes (aptitude)
    cDicoApt * Dico();//{return mLay->Dico();}
protected:
    std::shared_ptr<layerBase> mLay;
    // key ; classe ou valeur, val ; nombre d'occurence
    std::map<std::string, int> mStat;
    std::map<std::string, int> mStatSimple;

    int mNbPix;
    std::string mMode; // fee vs cs
};



class cKKCS
{
public:
    cKKCS(std::string aCode,cDicoApt * aDico);
    ~cKKCS(){
        //std::cout << " destructeur  KKCS " << std::endl;
        mDico=NULL;
    }
    std::string Nom(){return mNom;}
    bool IsHabitat(){return mHabitat;}
    bool IsFact(){return (mCode!="Pot_norm" && !mHabitat);}
    bool IsPot(){return mCode=="Pot_norm";}
    std::string summary(){return "Potentiel et risque liés au stations : " +mCode + " , "+ mNom + " , "+ mNomCol
                +  " Echelle risque/pot zbio 1 station 1 : " + std::to_string(getEchelle(1,1));}
    std::string NomCarte();
    std::string shortNomCarte();

    std::string NomMapServerLayer();
    std::string NomMapServerLayerFull();

    int getEchelle(int aZbio,int aSTId);
    int getHab(int aZbio,int aSTId);
    std::map<int, color> getDicoCol(){return mDicoCol;}
    std::map<int, std::string> getDicoVal(){return mDicoVal;}
    std::map<int, std::string> * getDicoValPtr(){return &mDicoVal;}
    TypeCarte Type(){return mType;}

private:
    TypeCarte mType;
    cDicoApt * mDico;
    std::string mCode, mNom,mNomCol;
    int mId;
    bool mHabitat;
    // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station. Value ; echelle pour le potentiel, facteur eco ou risque
    std::map<int,std::map<int,int>> mEchelleCS;

    // bon pour les habitats ce n'est pas la même structure de donnée, mais je vais tout de même utiliser cet objet aussi
    std::map<int,std::map<int,std::vector<std::string>>> mHabitats;

    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> mDicoVal;
    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, color> mDicoCol;
};


class cEss
{
public:
    cEss(std::string aCodeEs,cDicoApt * aDico);

    //effectue la confrontation Apt Zbio et AptHydroTrophiue si hierarchique = true, sinon renvoie l'aptitude de l'écogramme
    int getApt(int aCodeNT, int aCodeNH, int aZbio, bool hierachique=true, int aTopo=666);
    // retourne l'aptitude global de la zone bioclimatique
    int getApt(int aZbio);
    // retourne l'aptitude du catalogue de station
    int getApt(int aZbio, int aSTId);
    bool hasCSApt(){
        bool aRes(1);
        if (mAptCS.size()==0) {
            aRes=0;
            //std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour CS" << std::endl;
        }
        return aRes;
    }
    bool hasFEEApt(){
        bool aRes(0);
        // maintenant j'initialise EcoVal comme une map de 10 elem vide
       /* if (mEcoVal.size()==0) {
            aRes=0;
            //std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour FEE" << std::endl;
        }*/
        if (mEcoVal.size()>0 && mEcoVal.at(1).size()>0) {
                    aRes=1;
                    //std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour FEE" << std::endl;
         }
        return aRes;
    }

    bool hasApt(){
        bool aRes(1);
        if (mEcoVal.size()==0 && mAptCS.size()==0) {
            aRes=0;
            std::cout << "essence " << mNomFR << " n'as pas d'aptitude pour FEE ni pour Catalogue de stations" << std::endl;
        }
        return aRes;
    }

    int getFinalApt(int aCodeNT,int aCodeNH, int aZbio, int topo){
        //int apt=getApt(aCodeNT, aCodeNH,aZbio);
        //return corrigAptRisqueTopo(apt,topo,aZbio);
        return getApt(aCodeNT,aCodeNH,aZbio,true,topo);
    }

    //int corrigAptRisqueTopo(int apt, int topo, int zbio);
    // renvoie l'apt climatique compensée par situation topographique
    int corrigAptBioRisqueTopo(int aptBio,int topo,int zbio);

    std::string Nom(){return mNomFR;}
    std::string Code(){return mCode;}

    std::string NomCarteAptFEE();
    std::string shortNomCarteAptFEE();
    std::string NomDirTuileAptFEE();
    std::string NomCarteAptCS();
    std::string shortNomCarteAptCS();
    std::string NomDirTuileAptCS();

    std::string NomMapServerLayer();
    std::string NomMapServerLayerFull();//pour FEE et CS
    std::string NomMapServerLayerCS();


    // aptitude ecograme : clé chaine charactère ; c'est la combinaison ntxnh du genre "A2p5" ou "Mm4
    std::map<int,std::map<std::string,int>> mEcoVal;
    // aptitude pour chaque zone bioclim
    std::map<int,int> mAptZbio;
    // aptitude pour catalogue de station
    // clé ; zone bioclim/ région. Value ; une map -> clé = identifiant de la station. Value ; aptitude
    std::map<int,std::map<int,int>> mAptCS;
    // clé ; zone bioclim/ région. Value ; une map -> clé ; id situation topo. valeur ; code risque
    std::map<int,std::map<int,int>> mRisqueTopo;

    int getRisque(int zbio,int topo){
        int aRes(0);
        if (mRisqueTopo.find(zbio)!=mRisqueTopo.end() && mRisqueTopo.at(zbio).find(topo)!=mRisqueTopo.at(zbio).end()){
            aRes=mRisqueTopo.at(zbio).at(topo);
        }
        return aRes;
    }
    // savoir si il faut utiliser la situation topo comme facteur de compensation ou d'aggravation
    bool hasRisqueComp(int zbio,int topo);

    cDicoApt * Dico(){return mDico;}
    std::string printRisque();

    TypeCarte Type(){return mType;}

private:
    FeRe mFeRe;
    TypeCarte mType;
    cDicoApt * mDico;
    std::string mCode, mNomFR, mF_R,mPrefix;
};


#endif // LAYERBASE_H
