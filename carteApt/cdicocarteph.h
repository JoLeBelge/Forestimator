#ifndef CDICOCARTEPH_H
#define CDICOCARTEPH_H
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

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Sqlite3.h>


namespace dbo = Wt::Dbo;

//juin 2021 je vais refaire le code pour carte NT ici aussi, et utiliser ce dictionnaire
class siglePedo;


class siglePedo{
public:
    siglePedo(){}
    siglePedo(dbo::ptr<siglePedo> p){
        INDEX_=p->getIndex();
        SUBSTRAT=p->getSUBSTRAT();
        mMAT_TEXT=p->getMAT_TEXT();
        mPHASE_1=p->getPHASE_1();
        mPHASE_2=p->getPHASE_2();
        mPHASE_4=p->getPHASE_4();
        mPHASE_6=p->getPHASE_6();

        mCHARGE=p->getCHARGE();
        mDRAINAGE=p->getDRAINAGE();
        mDEV_PROFIL=p->getDEV_PROFIL();
        mSER_SPEC=p->getSER_SPEC();
        prepare();
    }
    int getIndex() const {return INDEX_;}
    std::string getSUBSTRAT() const {return SUBSTRAT;}
    std::string getMAT_TEXT() const {return mMAT_TEXT;}
    std::string getPHASE_1() const {return mPHASE_1;}
    std::string getPHASE_2() const {return mPHASE_2;}
    std::string getPHASE_4() const {return mPHASE_4;}
    std::string getPHASE_6() const {return mPHASE_6;}
    std::string getCHARGE() const {return mCHARGE;}

    std::string getDRAINAGE() const {return mDRAINAGE;}
    std::string getDEV_PROFIL() const {return mDEV_PROFIL;}
    std::string getSER_SPEC() const {return mSER_SPEC;}

    /* bool calcaire() const;
    // serie.special.riche
    bool ssriche() const;
    bool profond() const;
    bool alluvion() const;
    bool podzol() const;
     bool podzolique() const;
    bool superficiel() const;
    bool tourbe() const;
    bool limon() const;
    */
    bool calcaire() const {return mCalcaire;}
    // serie.special.riche
    bool ssriche() const {return mSsriche;}
    bool profond() const{return mProfond;}
    bool alluvion() const{return mAlluvion;}
    bool podzol() const{return mPodzol;}
    bool podzolique() const{return mPodzolique;}
    bool superficiel() const{return mSuperficiel;}
    bool tourbe() const{return mTourbe;}
    bool limon() const{return mLimon;}
    bool chargeSchisteux() const{return SUBSTRAT=="f";}

    void cat() const{std::cout << " siglePedo " << INDEX_ << " , substrat " << SUBSTRAT << ", drainage " << mDRAINAGE << std::endl;}

    void prepare();
    template<class Action>
    void persist(Action& a)
    {
        dbo::field(a, INDEX_,     "INDEX_");
        dbo::field(a, SUBSTRAT,     "SUBSTRAT");
        dbo::field(a, mMAT_TEXT, "MAT_TEXT");
        dbo::field(a, mPHASE_1, "PHASE_1");
        dbo::field(a, mPHASE_2, "PHASE_2");
        dbo::field(a, mPHASE_4, "PHASE_4");
        dbo::field(a, mPHASE_6, "PHASE_6");
        dbo::field(a, mCHARGE,     "CHARGE");
        dbo::field(a, mDRAINAGE,    "DRAINAGE");
        dbo::field(a, mDEV_PROFIL,    "DEV_PROFIL");
        dbo::field(a, mSER_SPEC,    "SER_SPEC");
    }
private:
    bool mCalcaire,mSsriche,mProfond,mAlluvion,mPodzol,mPodzolique,mSuperficiel,mTourbe,mLimon;
    std::string SUBSTRAT,mMAT_TEXT, mPHASE_1, mPHASE_2,mPHASE_4, mPHASE_6, mCHARGE, mDRAINAGE, mDEV_PROFIL, mSER_SPEC;
    int INDEX_;


};

class cDicoCartepH
{
public:
    cDicoCartepH(std::string aBDFile);
    void closeConnection();

    double getpH(int aPTS,int aZBIO);
    double getpH(int aPTS){
        double aRes(1.0);
        if (Dico_PTS2pH.find(aPTS)!=Dico_PTS2pH.end()){aRes=Dico_PTS2pH.at(aPTS);}
        return aRes;
    }
    std::string File(std::string aCode){
        std::string aRes("");
        if (Dico_GISfile.find(aCode)!=Dico_GISfile.end()){aRes=Dico_GISfile.at(aCode);}
        return aRes;
    }
    double toDouble(std::string aStr);

    int getPTS(int aSolIndex){
        int aRes(0);
        if (Dico_IndexSiglePed2PTS.find(aSolIndex)!=Dico_IndexSiglePed2PTS.end()){aRes=Dico_IndexSiglePed2PTS.at(aSolIndex);}
        return aRes;
    }

    /*dbo::ptr<siglePedo> getSiglePedoPtr(int aIndex){

        std::cout << "get Sigle pour " << aIndex << std::endl;
        dbo::ptr<siglePedo> aRes;
        if (mMSigles.find(aIndex)!=mMSigles.end()){
            aRes=mMSigles.at(aIndex);
        }
        // d'abord regarder dans la map si je l'ai déjà quelque part
        if(mMSigles.find(aIndex)!=mMSigles.end()){
            aRes=mMSigles.at(aIndex).get();
        } else {
        dbo::Transaction transaction{session};
        dbo::ptr<siglePedo> aRes = session.find<siglePedo>().where("INDEX_=?").bind(std::to_string(aIndex));
        mMSigles.emplace(std::make_pair(aIndex,aRes));
        }

        return aRes;
    }*/

    siglePedo * getSiglePedoPtr(int aIndex){

        //std::cout << "get Sigle pour " << aIndex << std::endl;
        siglePedo * aRes;

        // d'abord regarder dans la map si je l'ai déjà quelque part
        if(mMSigles.find(aIndex)!=mMSigles.end()){
            aRes=mMSigles.at(aIndex);
        } else {
            dbo::Transaction transaction{session};
            dbo::ptr<siglePedo> s = session.find<siglePedo>().where("INDEX_=?").bind(std::to_string(aIndex));

            siglePedo * s2=new siglePedo(s);
            mMSigles.emplace(std::make_pair(aIndex,s2));
            aRes=s2;
        }

        return aRes;
    }

private:
    std::string mBDpath;
    sqlite3 *db_;

    std::map<std::string,int> Dico_NomRN2Code;
    std::map<int,std::string> Dico_codeRN2Nom;
    std::map<int,std::string> Dico_PTS;
    std::map<int,int> Dico_Zbio2RN;
    std::map<int,int> Dico_IndexSiglePed2PTS;
    std::map<int,double> Dico_PTS2pH;
    std::map<std::vector<int>,double> Dico_PTSetRN2pH;
    // le nombre de mesure - en dessous de 2 on prends pas (correction 2021 06)
    std::map<std::vector<int>,int> Dico_PTSetRN2NbpH;


    std::map<std::string,std::string>  Dico_GISfile;

    // clé; index, val; l'objet sigle pédo avec tout dedans
    std::map<int,siglePedo*> mMSigles;
    //std::vector<dbo::ptr<siglePedo>> mVSigles;
    // dico texture vers texture simplifiée
    std::map<std::string,std::string> Dico_MAT_TEXT;

    dbo::Session session;


};



#endif // CDICOCARTEPH_H
