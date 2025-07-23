#ifndef CDICOAPT_H
#define CDICOAPT_H
#include "cdicoaptbase.h"
#include  "cnsw.h"
#include "layerbase.h"
#include "cadastre.h"

std::string removeAccents(std::string aStr);
std::string putInBalise(std::string aCont,std::string aBalise);

//extern std::string dirBD;

class cdicoAptBase;
class color;
class cDicoApt;
class layerBase;
class ST;
class cnsw;
class cadastre;

enum typeAna {ponctuel,surfacique,dicoTable};

extern int globMaxSurf;

// toute les informations/ dico que j'ai besoin pour le soft
class cDicoApt : public cdicoAptBase
{
public:
    // charger les dicos depuis BD SQL
    cDicoApt(std::string aBDFile);

    std::shared_ptr<cnsw> mPedo;
    std::shared_ptr<cadastre> mCadastre;

    /*************************/
    std::string geoservice(std::string aTool,std::string aArgs,std::string aPolyg,typeAna aType,bool xml=0);
    bool checkTool(std::string aTool);
    OGRLayer * uploadLayer(std::string aShpToUpload);
    OGRGeometry * checkPolyg(std::string aPolyg, int maxSurf=globMaxSurf);
    OGRPoint * checkPoint(std::string aPolyg);
    // pour commencer, uniquement une liste de code de carte d'aptitude (AG_FEE, ...)
    std::vector<std::string> parseAptArg(std::string aArgs);
    std::map<int,double> simplifieAptStat(std::map<int,double> aStat);
    std::vector<std::string> parseHdomArg(std::string aArgs);
    std::vector<std::string> parseCompoArg(std::string aArgs);
    std::string parsePointArg(std::string aArgs);
    /*************************/
    std::map<int,std::string> getDicoRaster(std::string aCode);
    std::map<std::string,std::string>  * Files(){return  &Dico_GISfile;}
    // code carte vers type carte code : NH.tif
    std::map<std::string,std::string>  * RasterType(){return  &Dico_RasterType;}
    std::map<std::string,std::string>  * RasterVar(){return  &Dico_RasterVar;}
    std::map<std::string,std::string>  * RasterCategorie(){return  &Dico_RasterCategorie;}
    std::map<std::string,std::string>  * RasterNom(){return  &Dico_RasterNomComplet;}
    std::map<std::string,bool>  * RasterExpert(){return  &Dico_RasterExpert;}

    std::map<int, std::shared_ptr<color>>  codeApt2col(){return  Dico_codeApt2col;}

    std::string BDpath(){return mBDpath;}
    std::string RasterNom(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterNomComplet.find(aCode)!=Dico_RasterNomComplet.end()){aRes=Dico_RasterNomComplet.at(aCode);}
        return aRes;
    }
    std::string RasterNomCourt(std::string aCode){
        std::string aRes("not found");
        if (Dico_RasterNomCourt.find(aCode)!=Dico_RasterNomCourt.end()){aRes=Dico_RasterNomCourt.at(aCode);}
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
    double RasterGain(std::string aCode){
        double aRes(1);
        if (Dico_RasterGain.find(aCode)!=Dico_RasterGain.end()){aRes=Dico_RasterGain.at(aCode);}
        return aRes;
    }
    double RasterResolution(std::string aCode){
        double aRes(10.0);
        if (Dico_RasterResolution.find(aCode)!=Dico_RasterResolution.end()){aRes=Dico_RasterResolution.at(aCode);
        }
        return aRes;
    }

    std::string station(int aZbio, int aSt,std::string aVar="a"){
        std::string aRes("not found");
        if (Dico_station.find(aZbio)!=Dico_station.end()){
            aRes="Pas de station numéro " + std::to_string(aSt) + " pour cette zone bioclimatique";
            if (Dico_station.at(aZbio).find(std::make_tuple(aSt,""))!=Dico_station.at(aZbio).end()){
                aRes=Dico_station.at(aZbio).at(std::make_tuple(aSt,""));
            }
            if (Dico_station.at(aZbio).find(std::make_tuple(aSt,aVar))!=Dico_station.at(aZbio).end()){
                aRes=Dico_station.at(aZbio).at(std::make_tuple(aSt,aVar));
            }
        }
        return aRes;
    }

    std::string stationEtVar(int aZbio, int aSt,std::string aVar="a"){
        std::string aRes=station(aZbio,aSt,aVar);
        if (Dico_station_varName.find(aZbio)!=Dico_station_varName.end()){
            if (Dico_station_varName.at(aZbio).find(std::make_tuple(aSt,aVar))!=Dico_station_varName.at(aZbio).end()){
                aRes+=" "+Dico_station_varName.at(aZbio).at(std::make_tuple(aSt,aVar));
            }
        }
        return aRes;
    }

    std::map<std::tuple<int, std::string>,std::string> aVStation(int aZbio){
        std::map<std::tuple<int, std::string>,std::string> aRes;
        if (Dico_station.find(aZbio)!=Dico_station.end()){
                aRes=Dico_station.at(aZbio);
        }
        return aRes;
    }

    std::string rasterCat(std::string aCode){
        std::string aRes("");
        if (Dico_RasterCategorie.find(aCode)!=Dico_RasterCategorie.end()){aRes=Dico_RasterCategorie.at(aCode);}
        return aRes;
    }

    std::string codeKK2Nom(std::string aCode){
        std::string aRes("not found\n");
        if (Dico_codeKK2Nom.find(aCode)!=Dico_codeKK2Nom.end()){aRes=Dico_codeKK2Nom.at(aCode);}
        return aRes;
    }

    int orderContrainteApt(int aCode){
        int aRes(12);
        if (Dico_Apt2OrdreContr.find(aCode)!=Dico_Apt2OrdreContr.end()){aRes=Dico_Apt2OrdreContr.at(aCode);}
        return aRes;
    }

    // hauteur en mètres de la couche MNH2019 que j'ai convertie en 8bits
    double H(int aVal,double aGain=0.2){
        double aRes(0.0);
        // attention, jai des MNH en 16bit donc on peut dépasser 255
        //if (aVal<255 && aVal>0){aRes=aVal*aGain;}
        if (aVal>0){aRes=aVal*aGain;}
        //std::cout << " hauteur DN " << aVal << " devient " << aRes << std::endl;
        return aRes;
    }

    // hauteur en mètres de la couche MNT que j'ai convertie en 16bits
    /*double mnt(int aVal){
        double aRes(aVal/10);
        return aRes;
    }*/

    bool hasWMSinfo(std::string aCode);

    WMSinfo * getWMSinfo(std::string aCode);

    //std::map<int,std::map<int,std::vector<std::string>>> getHabitatCS(std::string aColName);

    // pour les cRasterInfo, carte thématique
    std::map<int, std::shared_ptr<color> > getDicoRasterCol(std::string aCode);

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
    bool lay4StatP(std::string aLayerCode){
        bool aRes(1);
        if (Dico_RasterStatP.find(aLayerCode)!=Dico_RasterStatP.end()){aRes=Dico_RasterStatP.at(aLayerCode);}
        return aRes;
    }

    std::string lay2groupe(std::string aLayCode){
        std::string aRes("REF");
        if (Dico_lay2groupe.find(aLayCode)!=Dico_lay2groupe.end()){aRes=Dico_lay2groupe.at(aLayCode);}
        return aRes;
    }

    std::string groupeLabel(std::string aGroupeCode){
        std::string aRes("REF");
        if (Dico_groupeLabel.find(aGroupeCode)!=Dico_groupeLabel.end()){aRes=Dico_groupeLabel.at(aGroupeCode);}
        return aRes;
    }
    bool groupeExpert(std::string aGroupeCode){
        bool aRes(1);
        if (Dico_groupeExpert.find(aGroupeCode)!=Dico_groupeExpert.end()){aRes=Dico_groupeExpert.at(aGroupeCode);}
        return aRes;
    }


    // pour debug
   /* void summaryRasterFile(){
        for (auto kv : Dico_GISfile){
            std::string code=kv.first;
            std::cout << "raster layer " << code << ", nom " << RasterNom(code) << ", fichier " << File(code) << " catégorie " << RasterCategorie(code) << std::endl;
        }
    }*/

    std::map<std::string,std::shared_ptr<layerBase>> VlayerBase(){return mVlayerBase;}
    bool hasLayerBase(std::string aCode){
        if (mVlayerBase.find(aCode)!=mVlayerBase.end()){return 1;} else {return 0;}
    }

    std::shared_ptr<layerBase> getLayerBase(std::string aCode){
        std::shared_ptr<layerBase> aRes=NULL;
        if (mVlayerBase.find(aCode)!=mVlayerBase.end()){aRes=mVlayerBase.at(aCode);} else {
            std::cout << " getLayerBase de cdicoapt, création d'une layerbase vide attention , code " << aCode << std::endl;
            aRes= std::make_shared<layerBase>("toto",this);
        }
        return aRes;
    }
    std::vector<std::shared_ptr<layerBase>> VlayersForGroupe(std::string aGroupe){
        std::vector<std::shared_ptr<layerBase>> aRes;
        for (auto & kv :mVlayerBase){
            if (lay2groupe(kv.first)==aGroupe){
            aRes.push_back(kv.second);
            }
        }
       return aRes;
   }
    // dans l'ordre que l'on souhaite!
    std::vector<std::string> Dico_groupe;

private:

    // code groupe 2 label groupe
    std::map<std::string,std::string> Dico_groupeLabel;

    std::map<std::string,std::shared_ptr<layerBase>> mVlayerBase;
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
