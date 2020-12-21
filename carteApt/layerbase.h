#ifndef LAYERBASE_H
#define LAYERBASE_H

#include "cdicoapt.h"
#include <boost/algorithm/string/replace.hpp>
#include "cpl_conv.h" // for CPLMalloc()
// pour les vecteurs
#include "ogrsf_frmts.h"
#include "gdal_utils.h"
#include <numeric>

// dans Forestimator, un gros problème, c'est le fait que j'ai ma classe layer qui soiet strictement liée à Wt et à Forestimator (via grouplayer)
// J'aurai du avoir une classe mère qui soie indépendante de Wt et me permette de faire des stats sur des cartes, exactement comme j'en ai besoin maintenant pour stationDescriptor
// je tente donc de faire ça en repartant de cRasterInfo qui était un bon début

class LayerMTD; // classe tout à fait à par qui sert uniquement à générer la rubrique de documentation de forestimator pour cette couche
class WMSinfo;
class basicStat;

class cEss; // avec les aptitudes de l'essence
class cKKCS; // ce qui caractérise les stations ; potentiel sylvicole, facteur écologique, risques


class rasterFiles; // une classe dédiée uniquemnent à l'export et au clip des couches.
// rasterinfo hérite de rasterFile
class cRasterInfo;
// layerBase hérite de rasterInfo
class layerBase;
// le mieux aurai été de n'avoir que 2 classe, une +- = à rasterFile et une = à layerbase, mais vu que j'avais fait ça comme un cochon et que je vais pas tout changer d'un coup, j'en laisse 3

// ça c'était utilisé pour le calcul des carte apt, des tuiles et du code mapserveur
enum TypeCarte {Apt, Potentiel, Station1, Habitats,NH,NT,Topo,AE,SS,ZBIO,CSArdenne,CSLorraine,MNH2019,Composition,MNT16b};



enum class TypeVar {Classe,
                    Continu
                   };

enum class TypeLayer {
    KK // les cartes dérivées des CS
    ,Station // lié à la description de la station, approche FEE ; NH NT ZBIO
    ,Externe // toutes les cartes qui ne sont pas en local ; carte IGN pour commencer
    ,FEE // aptitude
    ,CS // aptitude
    ,Peuplement // description du peuplement en place
};

// forward dec
class cDicoApt;
class color;


inline bool exists (const std::string& name){
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

TypeCarte str2TypeCarte(const std::string& str);
TypeVar str2TypeVar(const std::string& str);
TypeLayer str2TypeLayer(const std::string& str);

std::string roundDouble(double d, int precisionVal=1);

class basicStat{
public:

    basicStat():min(0),max(0),mean(0),stdev(0),nb(0){}
    basicStat(std::map<double,int> aMapValandFrequ);
    basicStat(std::vector<double> v);
    std::string getMin(){return roundDouble(min);}
    std::string getMax(){return roundDouble(max);}
    std::string getMean(){return roundDouble(mean);}
    std::string getNb(){return std::to_string(nb);}
    std::string getSd(){return roundDouble(stdev);}
    std::string getCV(){if (mean!=0) {return roundDouble(100.0*stdev/mean)+"%";} else { return "-1";};}
   private:
    double min,max,mean,stdev;
    int nb;
};


class WMSinfo
{
public:
    WMSinfo():mUrl(""),mWMSLayerName("toto"){}
    WMSinfo(std::string url,std::string layer, std::string attribution=""):mUrl(url),mWMSLayerName(layer),mWMSattribution(attribution){}
    std::string mUrl, mWMSLayerName, mWMSattribution;
};


// pour afficher en html ou pdf les informations relatives aux couches, aux stations des cs, au méthodologies (genre calcul de hdom)
class LayerMTD{
public:
    LayerMTD(){}
    void setNom(std::string aNom){mNom=aNom;}
    void setLabel(std::string a){mShortName=a;}
    void setProjet(std::string aProj){mProjet=aProj;}
    void setDescr(std::string aDesc){mDescr=aDesc;}
    void setVersion(std::string aV){mVersion=aV;}
    void addRef(std::string aVRef){mVRefs.push_back(aVRef);}
    void setCopyR(std::string a){mCopyR=a;}
    std::string Nom(){return mNom;}
    std::string Label(){return mShortName;}
    std::string Descr(){return mDescr;}
    std::string Vers(){return mVersion;}
    std::string Projet(){return mProjet;}
    std::string CopyR(){return mCopyR;}
    std::vector<std::string> VRefs(){return mVRefs;}
    //std::string getHtml(); je fait une fonction dans grouplayer pour ne pas avoir de dépendance wt dans le dicoApt (utilisé par ailleurs dans des projets non-wt)
private:
    std::vector<std::string> mVRefs;
    std::string mProjet, mDescr,mVersion, mNom,mCopyR,mShortName;
};

class rasterFiles{
public:
    rasterFiles(std::string aPathTif,std::string aCode);
    std::string Code() const{return mCode;}
    void checkForQml();
    // ma classe rasterInfo hérite de rf mais ne sais pas l'instantier correctement dans la liste d'instantiation car dois questionner le dico pour savoir le chemin d'accès
    //void lateConstructor(std::string aPathTif,std::string aCode);
    // retourne le chemin d'accès complêt
    std::string getPathTif() const{return mPathRaster;}
    std::string symbology() const{return mPathQml;}
    bool hasSymbology() const{return mPathQml!="";}
    rasterFiles getRasterfile();
protected:
    std::string mPathRaster, mPathQml, mCode;
};

class cRasterInfo : public rasterFiles
{
public:
    cRasterInfo(std::string aCode,cDicoApt * aDico);
    std::string NomFile(); // nom du fichier tiff sans l'extension
    std::string NomFileWithExt();
    std::string Nom(){return mNom;}
    TypeCarte TypeCart(){return mTypeCarte;}
    std::map<int, std::string> * getDicoVal(){return &mDicoVal;}
    std::map<int, color>  getDicoCol(){return mDicoCol;}

    TypeVar getTypeVar() const{return mTypeVar;}
    TypeLayer getCatLayer() const{return mType;}
    bool Expert() const{return mExpert;}
    void catSummary(){std::cout << "RasterInfo ; Code " << mCode << " , Nom " << mNom << ", raser " << mPathRaster << ", dictionnaire valeurs de " << mDicoVal.size() << " elements " << std::endl;}

protected:
    TypeCarte mTypeCarte;
    TypeVar mTypeVar; // var continue ou discontinue, pour le calcul de statistique
    TypeLayer mType;
    cDicoApt * mDico;
    std::string mNom;
    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> mDicoVal;

    // le dictionnaire des valeurs raster vers la couleur.
    std::map<int, color> mDicoCol;
    // pour distinguer les cartes qui sont accèssible à tous ou pas
    bool mExpert;
};




class layerBase : public cRasterInfo
{
public:

    layerBase(std::string aCode,cDicoApt * aDico);

    // méthode GDAL
    int getValue(double x, double y);

    // stat sur un polygone ; deux retour possible, une map avec clé = valeur raster, une map avec clé = signification string
    // clé ; signification du code raster. Val ; nombre d'occurence
    std::map<std::string,int> computeStat1(OGRGeometry * poGeom);
    // clé ; val raster. val ; pct. Comment gerer les no data? code int -1?
    std::map<int,double> computeStat2(OGRGeometry * poGeom);
    // retourne la valeur majoritaire sur le polygone ainsi que son prct en surface
    std::pair<int,double> valMajoritaire(OGRGeometry * poGeom);
    GDALDataset * rasterizeGeom(OGRGeometry *poGeom);
    // ça c'est pour les couches variables continues
    basicStat computeBasicStatOnPolyg(OGRGeometry * poGeom);
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
    std::string NomDirTuile();
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
    int getApt(int aCodeNT,int aCodeNH, int aZbio,bool hierachique=true);
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
        int apt=getApt(aCodeNT, aCodeNH,aZbio);
        return corrigAptRisqueTopo(apt,topo,aZbio);
    }

    int corrigAptRisqueTopo(int apt, int topo, int zbio);
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
    TypeCarte mType;
    cDicoApt * mDico;
    std::string mCode, mNomFR, mF_R,mPrefix;
};


#endif // LAYERBASE_H
