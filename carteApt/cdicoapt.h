#ifndef CDICOAPT_H
#define CDICOAPT_H
#include <sqlite3.h>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/map.hpp>
#include "boost/filesystem.hpp"
#include <unistd.h>
#include <cmath>
#include  "cnsw.h"

enum TypeCarte {Apt, Potentiel, Station1, Habitats,NH,NT,Topo,AE,SS,ZBIO,CSArdenne,CSLorraine,MNH2019,Composition,MNT16b};

enum class TypeVar {Classe,
                    Continu
                   };

enum class TypeLayer { //Apti // Apti enlevé ! -> FEE et CS
    KK // les cartes dérivées des CS
    ,Thematique // lié à la description de la station, approche FEE ; NH NT ZBIO
    ,Externe // toutes les cartes qui ne sont pas en local ; carte IGN pour commencer
    ,FEE // aptitude
    ,CS // aptitude
    ,Peuplement // description du peuplement en place
};

std::string loadBDpath();

std::string removeAccents(std::string aStr);

TypeCarte str2TypeCarte(const std::string& str);
TypeVar str2TypeVar(const std::string& str);
TypeLayer str2TypeLayer(const std::string& str);

extern std::string dirBD;
class WMSinfo;
class color;
class cDicoApt;
class cEss; // avec les aptitudes de l'essence
class cKKCS; // ce qui caractérise les stations ; potentiel sylvicole, facteur écologique, risques
class cRasterInfo; // ça aurait du être une classe mère ou membre de cEss et cKKCS mais je l'implémente après, c'est pour avoir les info à propose des rasters FEE ; NT, NH, Topo, AE, SS
class ST;
class cnsw;
class LayerMTD;


class WMSinfo
{
public:
    WMSinfo():mUrl(""),mWMSLayerName("toto"){}
    WMSinfo(std::string url,std::string layer):mUrl(url),mWMSLayerName(layer){}
    std::string mUrl, mWMSLayerName;
};

// pour afficher en html ou pdf les informations relatives aux couches, aux stations des cs, au méthodologies (genre calcul de hdom)
class LayerMTD{
public:
    LayerMTD(){}
    void setNom(std::string aNom){mNom=aNom;}
    void setProjet(std::string aProj){mProjet=aProj;}
    void setDescr(std::string aDesc){mDescr=aDesc;}
    void setVersion(std::string aV){mVersion=aV;}
    void addRef(std::string aVRef){mVRefs.push_back(aVRef);}
    std::string Nom(){return mNom;}
    std::string Descr(){return mDescr;}
    std::string Vers(){return mVersion;}
    std::string Projet(){return mProjet;}
    std::vector<std::string> VRefs(){return mVRefs;}
    //std::string getHtml(); je fait une fonction dans grouplayer pour ne pas avoir de dépendance wt dans le dicoApt (utilisé par ailleurs dans des projets non-wt)
private:
    std::vector<std::string> mVRefs;
    std::string mProjet, mDescr,mVersion, mNom;
};

class color
{
public:
    color(int R,int G,int B,std::string name="toto"):mR(R),mG(G),mB(B),mStyleClassName(name){isDark();}
    color(std::string aHex,std::string name):mStyleClassName(name){
        // j'enlève le diaise qui semble ne pas convenir
        const char* c=aHex.substr(1,aHex.size()).c_str();
        sscanf(c, "%02x%02x%02x", &mR, &mG, &mB);
        //std::cout << std::to_string(mR) << ";" <<std::to_string(mG) << ";" <<std::to_string(mB) << std::endl;
        isDark();
    }
    color(std::string aHex){
        const char* c=aHex.substr(1,aHex.size()).c_str();
        // j'enlève le diaise qui semble ne pas convenir pour le nom de style il faut également s'assurer que le code ne commence pas par un numéro.
        mStyleClassName=aHex.substr(1,aHex.size());
        if (isdigit(mStyleClassName[0])){ mStyleClassName="a"+mStyleClassName;}
        sscanf(c, "%02x%02x%02x", &mR, &mG, &mB);
        //std::cout << std::to_string(mR) << ";" <<std::to_string(mG) << ";" <<std::to_string(mB) << std::endl;
        isDark();
    }
    int mR,mG,mB;
    void set(int &R,int &G,int &B){
        R=mR;
        G=mG;
        B=mB;
        isDark();
    }

    void isDark(){
        double hsp = 0.299 * pow(mR,2) + 0.587 * pow(mG,2) + 0.114 * pow(mB,2);
        //if (hsp<127.5) {mDark=true;} else {mDark=false;}
        if (hsp<170) {mDark=true;} else {mDark=false;}
    }

    bool dark(){return mDark;}

    std::string cat(){ return " R:" + std::to_string(mR)+", G:"+std::to_string(mG)+", B"+std::to_string(mB);}
    std::string cat2(){ return std::to_string(mR)+" "+std::to_string(mG)+" "+std::to_string(mB);}
    std::string catHex(){
        unsigned long hex= ((mR & 0xff) << 16) + ((mG & 0xff) << 8) + (mB & 0xff);
        return "#"+std::to_string(hex);
    }
    std::string getStyleName(){return "."+mStyleClassName;}
    std::string getStyleNameShort(){return mStyleClassName;}
    std::string mStyleClassName;
    bool mDark;
};


class cRasterInfo
{
public:
    cRasterInfo(std::string aCode,cDicoApt * aDico);
    std::string NomFile(); // nom du fichier tiff sans l'extension
    std::string NomFileWithExt();
    std::string Nom(){return mNom;}
    std::string Code(){return mCode;}
    std::string NomCarte(){return mPathRaster;}// retourne le chemin d'accès complêt
    std::string NomDirTuile();
    std::string NomTuile();
    TypeCarte Type(){return mType;}
    std::map<int, std::string> * getDicoVal(){return &mDicoVal;}
    std::map<int, color>  getDicoCol(){return mDicoCol;}

    TypeVar getTypeVar() const{return mTypeVar;}
    TypeLayer getCatLayer() const{return mTypeLayer;}
    bool Expert() const{return mExpert;}

private:
    TypeCarte mType;
    TypeVar mTypeVar; // var continue ou discontinue, pour le calcul de statistique
    TypeLayer mTypeLayer;
    cDicoApt * mDico;
    std::string mCode, mNom,mPathRaster;
    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, std::string> mDicoVal;

    // le dictionnaire des valeurs raster vers leur signification.
    std::map<int, color> mDicoCol;
    // pour distinguer les cartes qui sont accèssible à tous ou pas
    bool mExpert;
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
        bool aRes(1);
        if (mEcoVal.size()==0) {
            aRes=0;
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

// toute les informations/ dico que j'ai besoin pour le soft
class cDicoApt
{
public:
    // charger les dicos depuis BD SQL
    cDicoApt(std::string aBDFile);

    cnsw * mPedo;

    void closeConnection();
    int openConnection();
    std::map<int,std::string> * ZBIO(){return  &Dico_ZBIO;}
    std::map<std::string,std::string>  * Files(){return  &Dico_GISfile;}
    // code carte vers type carte code : NH.tif
    std::map<std::string,std::string>  * RasterType(){return  &Dico_RasterType;}
    std::map<std::string,std::string>  * RasterVar(){return  &Dico_RasterVar;}
    std::map<std::string,std::string>  * RasterLayer(){return  &Dico_RasterLayer;}
    std::map<std::string,std::string>  * RasterNom(){return  &Dico_RasterNomComplet;}
    std::map<std::string,bool>  * RasterExpert(){return  &Dico_RasterExpert;}
    std::map<std::string,std::string>  * code2Nom(){return  &Dico_code2NomFR;}
    std::map<int,std::string>  * NH(){return  &Dico_NH;}
    std::map<int,std::string>  * NT(){return  &Dico_NT;}
    std::map<int,std::string>  * code2NTNH(){return  &Dico_code2NTNH;}
    std::map<std::string,int>  * NTNH(){return  &Dico_NTNH2Code;}
    std::map<int,std::string>  * code2Apt(){return  &Dico_code2Apt;}
    std::map<int,std::string>  * code2AptFull(){return  &Dico_code2AptFull;}
    std::map<std::string,int>  * Apt(){return  &Dico_Apt;}
    std::map<std::string,std::string>  * codeKK2Nom(){return  &Dico_codeKK2Nom;}
    std::map<std::string,std::string>  * codeKK2NomCol(){return  &Dico_codeKK2NomCol;}
    std::map<int,int>  * echelleFact(){return  &Dico_echelleFact;}
    std::map<int,std::string>  * echelleFactNom(){return  &Dico_echelleFactNom;}
    std::map<int,std::string>  * echellePotCat(){return  &Dico_echellePotCat;}

    std::map<std::string,std::string>  * codeSt2Habitat(){return  &Dico_codeSt2Habitat;}
    std::map<int,std::string>  * id2Hab(){return  &Dico_id2Habitat;}
    // clé : risque id. value; catégorie ID
    std::map<int,int>  * risqueCat(){return  &Dico_risqueCategorie;}

    std::map<int,std::string>  * topo(){return  &Dico_topo;}

    std::map<int, color>  codeApt2col(){return  Dico_codeApt2col;}

    std::string BDpath(){return mBDpath;}

    color Apt2col(int aCode){
        color aRes(0,0,0);
        if (Dico_codeApt2col.find(aCode)!=Dico_codeApt2col.end()){aRes=Dico_codeApt2col.at(aCode);}
        return aRes;
    }

    // Accès sécurisé aux dictionnaire
    std::string NT(int aCode){
        std::string aRes("not found");
        if (Dico_NT.find(aCode)!=Dico_NT.end()){aRes=Dico_NT.at(aCode);}
        return aRes;
    }
    std::string NH(int aCode){
        std::string aRes("not found");
        if (Dico_NH.find(aCode)!=Dico_NH.end()){aRes=Dico_NH.at(aCode);}
        return aRes;
    }

    int posEcoNH(int aCode){
        int aRes(0);
        if (Dico_NHposEco.find(aCode)!=Dico_NHposEco.end()){aRes=Dico_NHposEco.at(aCode);}
        return aRes;
    }
    int rasterNH2Gr(int aCode){
        int aRes(0);
        if (Dico_rasterNH2groupe.find(aCode)!=Dico_rasterNH2groupe.end()){aRes=Dico_rasterNH2groupe.at(aCode);}
        return aRes;
    }

    std::vector<int> NHGr(){
        std::vector<int> aRes ;
        for (auto kv : Dico_rasterNH2groupe){
            int gr = kv.second;
            std::vector<int>::iterator it = std::find(aRes.begin(), aRes.end(), gr);
            if (it == aRes.end()){
                aRes.push_back(gr);
            }
        }
        return aRes;
    }

    std::string RasterNom(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterNomComplet.find(aCode)!=Dico_RasterNomComplet.end()){aRes=Dico_RasterNomComplet.at(aCode);}
        return aRes;
    }
    bool RasterExpert(std::string aCode){
        bool aRes(0);
        if (Dico_RasterExpert.find(aCode)!=Dico_RasterExpert.end()){aRes=Dico_RasterExpert.at(aCode);}
        return aRes;
    }
    std::string RasterType(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterType.find(aCode)!=Dico_RasterType.end()){aRes=Dico_RasterType.at(aCode);}
        return aRes;
    }
    std::string RasterVar(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterVar.find(aCode)!=Dico_RasterVar.end()){aRes=Dico_RasterVar.at(aCode);}
        return aRes;
    }
    std::string RasterLayer(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterLayer.find(aCode)!=Dico_RasterLayer.end()){aRes=Dico_RasterLayer.at(aCode);}
        return aRes;
    }



    std::string ZBIO(int aCode){
        std::string aRes("not found");
        if (Dico_ZBIO.find(aCode)!=Dico_ZBIO.end()){aRes=Dico_ZBIO.at(aCode);}
        return aRes;
    }

    std::string TOPO(int aCode){
        std::string aRes("not found");
        if (Dico_topo.find(aCode)!=Dico_topo.end()){aRes=Dico_topo.at(aCode);}
        return aRes;
    }


    std::string code2NTNH(int aCode){
        std::string aRes("not found");
        if (Dico_code2NTNH.find(aCode)!=Dico_code2NTNH.end()){aRes=Dico_code2NTNH.at(aCode);}
        return aRes;
    }
    std::string station(int aZbio, int aSt){
        std::string aRes("not found");
        if (Dico_station.find(aZbio)!=Dico_station.end()){
            aRes="Pas de station numéro " + std::to_string(aSt) + " pour cette zone bioclimatique";
            if (Dico_station.at(aZbio).find(aSt)!=Dico_station.at(aZbio).end()){
                aRes=Dico_station.at(aZbio).at(aSt);
            }
        }
        return aRes;
    }
    std::string code2Apt(int aCode){
        std::string aRes("not found\n");
        if (aCode==20) aRes="Pas d'aptitude pour cette Zbio";
        if (aCode==0) aRes="Pas d'aptitude";
        if (Dico_code2Apt.find(aCode)!=Dico_code2Apt.end()){aRes=Dico_code2Apt.at(aCode);}
        return aRes;
    }

    std::string accroEss2Nom(std::string aCode){
        std::string aRes("");
        if (Dico_code2NomFR.find(aCode)!=Dico_code2NomFR.end()){aRes=Dico_code2NomFR.at(aCode);}
        return aRes;
    }

    std::string rasterCat(std::string aCode){
        std::string aRes("");
        if (Dico_RasterLayer.find(aCode)!=Dico_RasterLayer.end()){aRes=Dico_RasterLayer.at(aCode);}
        return aRes;
    }

    std::string accroEss2prefix(std::string aCode){
        std::string aRes("");
        if (Dico_code2prefix.find(aCode)!=Dico_code2prefix.end()){aRes=Dico_code2prefix.at(aCode);}
        return aRes;
    }

    std::string File(std::string aCode){
        std::string aRes("");
        if (Dico_GISfile.find(aCode)!=Dico_GISfile.end()){aRes=Dico_GISfile.at(aCode);}
        return aRes;
    }

    std::string code2AptFull(int aCode){
        std::string aRes("not found\n");
        if (aCode==20) aRes="Pas d'aptitude pour cette Zbio";
        if (aCode==0) aRes="Pas d'aptitude";
        if (Dico_code2AptFull.find(aCode)!=Dico_code2AptFull.end()){aRes=Dico_code2AptFull.at(aCode);}
        return aRes;
    }
    int Apt(std::string aCode){
        int aRes(777);
        if (Dico_Apt.find(aCode)!=Dico_Apt.end()){aRes=Dico_Apt.at(aCode);}
        return aRes;
    }
    int Risque(std::string aStr){
        int aRes(0);
        if (Dico_risque2Code.find(aStr)!=Dico_risque2Code.end()){aRes=Dico_risque2Code.at(aStr);}
        return aRes;
    }
    std::string Risque(int aCode){
        std::string aRes("pas de risque pour ce code");
        if (Dico_risque.find(aCode)!=Dico_risque.end()){aRes=Dico_risque.at(aCode);}
        return aRes;
    }
    int risqueCat(int aCode){
        int aRes(0);
        if (Dico_risqueCategorie.find(aCode)!=Dico_risqueCategorie.end()){aRes=Dico_risqueCategorie.at(aCode);}
        return aRes;
    }
    // on se débarasse des double aptitude en choisisant la plus contraignante
    int AptContraignante(int aCode){
        int aRes(0);
        if (Dico_AptDouble2AptContr.find(aCode)!=Dico_AptDouble2AptContr.end()){aRes=Dico_AptDouble2AptContr.at(aCode);}
        return aRes;
    }
    // on améliore l'aptitude car facteur de compensation

    int AptSurcote(int aCode){
        int aRes(aCode);
        if (Dico_AptSurcote.find(aCode)!=Dico_AptSurcote.end()){aRes=Dico_AptSurcote.at(aCode);}
        return aRes;
    }
    // on dégrade l'aptitude car facteur agravant
    int AptSouscote(int aCode){
        int aRes(aCode);
        if (Dico_AptSouscote.find(aCode)!=Dico_AptSouscote.end()){aRes=Dico_AptSouscote.at(aCode);}
        return aRes;
    }
    /* int AptCorrig(int aCode){
          int aRes(AptContraignante(aCode));
          if (aRes<4){aRes++;} else if (aRes>1){aRes--;}
          return aRes;
     }*/

    std::string codeKK2Nom(std::string aCode){
        std::string aRes("not found\n");
        if (Dico_codeKK2Nom.find(aCode)!=Dico_codeKK2Nom.end()){aRes=Dico_codeKK2Nom.at(aCode);}
        return aRes;
    }
    int echelleFact(int aCode){
        int aRes(0);
        if (Dico_echelleFact.find(aCode)!=Dico_echelleFact.end()){aRes=Dico_echelleFact.at(aCode);}
        return aRes;
    }

    int habitatId(std::string aCode){
        int aRes(0);
        if (Dico_codeSt2idHab.find(aCode)!=Dico_codeSt2idHab.end()){aRes=Dico_codeSt2idHab.at(aCode);}
        return aRes;
    }

    int orderContrainteApt(int aCode){
        int aRes(12);
        if (Dico_Apt2OrdreContr.find(aCode)!=Dico_Apt2OrdreContr.end()){aRes=Dico_Apt2OrdreContr.at(aCode);}
        return aRes;
    }

    // hauteur en mètres de la couche MNH2019 que j'ai convertie en 8bits
    double H(int aVal){
        double aRes(0.0);
        if (aVal<255 && aVal>0){aRes=aVal/5;}
        return aRes;
    }

    // hauteur en mètres de la couche MNT que j'ai convertie en 16bits
    double mnt(int aVal){
        double aRes(aVal/10);
        return aRes;
    }

    bool hasWMSinfo(std::string aCode){
        return Dico_WMS.find(aCode)!=Dico_WMS.end();
    }
    WMSinfo * getWMSinfo(std::string aCode){
        WMSinfo * aRes;
        if (Dico_WMS.find(aCode)!=Dico_WMS.end()){
            aRes=&Dico_WMS.at(aCode);
        };
        return aRes;
    }

    std::map<int,std::map<std::string,int>> getFEEApt(std::string aCodeEs);
    std::map<int,int> getZBIOApt(std::string aCodeEs);
    std::map<int,std::map<int,int>> getRisqueTopo(std::string aCodeEs);

    std::map<int,std::map<int,int>> getCSApt(std::string aCodeEs);
    // charger les valeurs pour les potentiel sylvi, facteur eco et risque pour chaque station
    std::map<int,std::map<int,int>> getKKCS(std::string aColName);

    std::map<int,std::map<int,std::vector<std::string>>> getHabitatCS(std::string aColName);

    std::map<int,std::string> getDicoRaster(std::string aCode);
    // pour les cRasterInfo, carte thématique
    std::map<int,color> getDicoRasterCol(std::string aCode);
    // pour les cKKCS,
    std::map<int,color> getDicoRasterCol(cKKCS * aKK);

    std::map<std::string,color> colors;

    color getColor(std::string aCode){
        color aRes(0,0,0);
        if (aCode.substr(0,1)=="#") {aRes=color(aCode);}
        else {
            if (colors.find(aCode)!=colors.end()){aRes=colors.at(aCode);}
        }
        return aRes;
    }

    bool hasMTD(std::string aLayerCode){
        return Dico_layerMTD.find(aLayerCode)!=Dico_layerMTD.end();
    }
    LayerMTD getLayerMTD(std::string aCode){
        LayerMTD aRes;
        if (Dico_layerMTD.find(aCode)!=Dico_layerMTD.end()){aRes=Dico_layerMTD.at(aCode);}
         return aRes;
    }

    std::map<std::string,LayerMTD> * layerMTD(){return &Dico_layerMTD;}

    std::map<std::string,std::string>  Dico_AptFull2AptAcro;// j'en ai besoin dans les batonnetApt
private:
    std::string mBDpath;

    //code ess vers nom français
    std::map<std::string,std::string> Dico_code2NomFR;
    std::map<std::string,std::string> Dico_code2prefix;
    // code essence 2 code groupe "feuillus" vs "Resineux
    std::map<std::string,std::string> Dico_F_R;
    std::map<std::string,std::string>  Dico_codeSt2Habitat;
    std::map<int,std::string>  Dico_id2Habitat;
    std::map<std::string,int> Dico_codeSt2idHab;

    std::map<std::string,LayerMTD>  Dico_layerMTD;
    std::map<std::string,std::string>  Dico_GISfile;
    std::map<std::string,std::string>  Dico_RasterType;
    // continu vs classe
    std::map<std::string,std::string>  Dico_RasterVar;
    // description peuplement vs description station
    std::map<std::string,std::string>  Dico_RasterLayer;
    std::map<std::string,std::string>  Dico_RasterNomComplet;
    std::map<std::string,bool>  Dico_RasterExpert;
    // key ; code le la couche layer. value ; les infos nécessaire pour charger le wms
    std::map<std::string,WMSinfo>  Dico_WMS;
    std::map<int,std::string>  Dico_ZBIO;
    std::map<int,std::string>  Dico_NH;
    // c'est dans mes analyses phyto que j'ai besoin de grouper les niveaux hydriques en groupe
    std::map<int,int>  Dico_rasterNH2groupe;
    // code NH vers position Y dans l'écogramme
    std::map<int,int>  Dico_NHposEco;
    std::map<int,std::string>  Dico_NT;
    std::map<int,std::string>  Dico_code2NTNH;
    std::map<std::string,int>  Dico_NTNH2Code;
    std::map<std::string,int>  Dico_Apt;
    std::map<int,std::string>  Dico_code2Apt;// apt sous forme d'acro, O, T, TE, ect

    std::map<int,std::string> Dico_code2AptFull;
    std::map<int,int>  Dico_AptDouble2AptContr;
    std::map<int,int>  Dico_AptSouscote;
    std::map<int,int>  Dico_AptSurcote;
    // les codes aptitudes sont classé dans un ordre fonction de la contrainte, permet de comparer deux aptitude
    std::map<int,int>  Dico_Apt2OrdreContr;
    // clé 1 : zbio, clé 2: id station,value ; nom de la sation cartograhique
    std::map<int,std::map<int,std::string>>  Dico_station;

    std::map<std::string,std::string> Dico_codeKK2Nom;
    std::map<std::string,std::string> Dico_codeKK2NomCol;
    // il y a 9 niveau dans l'échelle, mais on simplifie en 3 catégories pour les cartes de risque et potentiel sylv
    std::map<int,int> Dico_echelleFact;
    std::map<int,std::string> Dico_echelleFactNom;
    // de la catégorie ver le nom
    std::map<int,std::string> Dico_echellePotCat;

    std::map<int,std::string> Dico_risque;
    std::map<std::string,int> Dico_risque2Code;
    // on regroupe les 5 risques en 3 catégorie
    std::map<int,int> Dico_risqueCategorie;
    std::map<int,std::string>  Dico_topo;

    std::map<int,color> Dico_codeApt2col;

    sqlite3 *db_;

};

class ST{
public:
    ST(cDicoApt * aDico);
    void vider();
    std::string NH(){ return mDico->NH(mNH);}
    std::string NT(){ return mDico->NT(mNT);}
    std::string TOPO(){ return mDico->TOPO(mTOPO);}
    std::string ZBIO(){ return mDico->ZBIO(mZBIO);}
    std::string STATION(){return mDico->station(mZBIO,mSt);}

    bool hasNH(){ return (mDico->NH()->find(mNH)!=mDico->NH()->end()) && mNH!=0;}
    bool hasNT(){ return mDico->NT()->find(mNT)!=mDico->NT()->end();}
    bool hasZBIO(){ return mDico->ZBIO()->find(mZBIO)!=mDico->ZBIO()->end();}
    bool hasTOPO(){ return mDico->topo()->find(mTOPO)!=mDico->topo()->end();}
    bool hasST(){ return mDico->station(mZBIO,mSt)!="not found";}
    bool readyFEE(){ return hasNH() && hasNT() && hasZBIO() && hasTOPO();}
    bool readyCS(){ return hasZBIO() && hasST();}
    bool hasEss(){ return HaveEss;}
    double getX(){return x;}
    double getY(){return y;}
    void setX(double a){x=a;}
    void setY(double a){y=a;}
    void setHasFEEApt(bool a){hasFEEApt=a;}
    void setOK(){mEmpty=0;}
    bool ecogramme(){return hasFEEApt;}
    bool isOK(){return !mEmpty;}

    int mNH,mNT,mZBIO,mTOPO;
    bool HaveEss;
    cEss * mActiveEss; // l'essence qui intéresse l'utilisateur
    cDicoApt * mDico;
    // catalogue de station
    int mSt;


    OGRPoint getPoint(){
        OGRPoint pt(x,y);
        return pt;}
private:
    bool mEmpty;
    bool hasFEEApt;// pour savoir si l'écogramme est dessiné ou pas

    double x,y;

};
#endif // CDICOAPT_H
