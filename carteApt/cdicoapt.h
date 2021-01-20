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
#include "layerbase.h"
#include "color.h"

std::string loadBDpath();

std::string removeAccents(std::string aStr);


extern std::string dirBD;

class color;
class cDicoApt;
class layerBase; // ça aurait du être une classe mère ou membre de cEss et cKKCS mais je l'implémente après, c'est pour avoir les info à propose des rasters FEE ; NT, NH, Topo, AE, SS
class ST;
class cnsw;
class WMSinfo;
class LayerMTD;

class cEss;
class cKKCS;



// toute les informations/ dico que j'ai besoin pour le soft
class cDicoApt
{
public:
    // charger les dicos depuis BD SQL
    cDicoApt(std::string aBDFile);

    std::shared_ptr<cnsw> mPedo;

    void closeConnection();
    int openConnection();
    std::map<int,std::string> * ZBIO(){return  &Dico_ZBIO;}
    std::map<std::string,std::string>  * Files(){return  &Dico_GISfile;}
    // code carte vers type carte code : NH.tif
    std::map<std::string,std::string>  * RasterType(){return  &Dico_RasterType;}
    std::map<std::string,std::string>  * RasterVar(){return  &Dico_RasterVar;}
    std::map<std::string,std::string>  * RasterCategorie(){return  &Dico_RasterCategorie;}
    std::map<std::string,std::string>  * RasterNom(){return  &Dico_RasterNomComplet;}
    std::map<std::string,bool>  * RasterExpert(){return  &Dico_RasterExpert;}
    std::map<std::string,std::string>  * codeEs2Nom(){return  &Dico_codeEs2NomFR;}
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
    std::string RasterTable(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterTable.find(aCode)!=Dico_RasterTable.end()){aRes=Dico_RasterTable.at(aCode);}
        return aRes;
    }
    std::string RasterVar(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterVar.find(aCode)!=Dico_RasterVar.end()){aRes=Dico_RasterVar.at(aCode);}
        return aRes;
    }
    std::string RasterCategorie(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterCategorie.find(aCode)!=Dico_RasterCategorie.end()){aRes=Dico_RasterCategorie.at(aCode);}
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
        if (Dico_codeEs2NomFR.find(aCode)!=Dico_codeEs2NomFR.end()){aRes=Dico_codeEs2NomFR.at(aCode);}
        return aRes;
    }

    std::string rasterCat(std::string aCode){
        std::string aRes("");
        if (Dico_RasterCategorie.find(aCode)!=Dico_RasterCategorie.end()){aRes=Dico_RasterCategorie.at(aCode);}
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

    bool hasWMSinfo(std::string aCode);

    WMSinfo * getWMSinfo(std::string aCode);

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
    bool lay4Visu(std::string aLayerCode){
        bool aRes(0);
        if (Dico_RasterVisu.find(aLayerCode)!=Dico_RasterVisu.end()){aRes=Dico_RasterVisu.at(aLayerCode);}
        return aRes;
    }
    bool lay4Stat(std::string aLayerCode){
        bool aRes(1);
        if (Dico_RasterStat.find(aLayerCode)!=Dico_RasterStat.end()){aRes=Dico_RasterStat.at(aLayerCode);}
        return aRes;
    }

    /*bool hasMTD(std::string aLayerCode){
        return Dico_layerMTD.find(aLayerCode)!=Dico_layerMTD.end();
    }
    LayerMTD getLayerMTD(std::string aCode){
        LayerMTD aRes;
        if (Dico_layerMTD.find(aCode)!=Dico_layerMTD.end()){aRes=Dico_layerMTD.at(aCode);}
         return aRes;
    }*/

    std::map<std::string,LayerMTD> * layerMTD();

    std::map<std::string,std::string>  Dico_AptFull2AptAcro;// j'en ai besoin dans les batonnetApt

    // pour debug
    void summaryRasterFile(){
        for (auto kv : Dico_GISfile){
            std::string code=kv.first;
            std::cout << "raster layer " << code << ", nom " << RasterNom(code) << ", fichier " << File(code) << " catégorie " << RasterCategorie(code) << std::endl;
        }
    }

    std::shared_ptr<cEss> getEss(std::string aCode){
        std::shared_ptr<cEss> aRes=NULL;
        if (mVEss.find(aCode)!=mVEss.end()){aRes=mVEss.at(aCode);} else {
            std::cout << "getEss de cdicoapt, création d'une essence vide attention " << std::endl;
            aRes= std::make_shared<cEss>("toto",this);
        }
        return aRes;
    }

    std::map<std::string,std::shared_ptr<layerBase>> VlayerBase(){return mVlayerBase;}

    std::shared_ptr<layerBase> getLayerBase(std::string aCode){
        std::shared_ptr<layerBase> aRes=NULL;
        if (mVlayerBase.find(aCode)!=mVlayerBase.end()){aRes=mVlayerBase.at(aCode);} else {
            std::cout << " getLayerBase de cdicoapt, création d'une layerbase vide attention " << std::endl;
            aRes= std::make_shared<layerBase>("toto",this);
        }
        return aRes;
    }

private:
    std::string mBDpath;

    //code ess vers nom français
    std::map<std::string,std::string> Dico_codeEs2NomFR;
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
    std::map<std::string,bool>  Dico_RasterVisu;// les couches que l'on peux visualiser dans la partie carto
    std::map<std::string,bool>  Dico_RasterStat;// les couches sur lesquelles on peut calculer des statistiques
    // description peuplement vs description station
    std::map<std::string,std::string>  Dico_RasterCategorie;
    std::map<std::string,std::string>  Dico_RasterNomComplet;
    std::map<std::string,bool>  Dico_RasterExpert;
    // pour savoir de quelle table provient les info du raster, fichiersGIS ou layerApt, car j'ai besoin du nom de la table pour charger le dicitonnaire (pour l'instant)
    std::map<std::string,std::string>  Dico_RasterTable;
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

    // clé ; code ess. val ; pointeur vers essence
    std::map<std::string,std::shared_ptr<cEss>> mVEss;
    std::map<std::string,std::shared_ptr<layerBase>> mVlayerBase;



    sqlite3 **db_;
    sqlite3 * ptDb_;

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
    std::shared_ptr<cEss> mActiveEss; // l'essence qui intéresse l'utilisateur
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
