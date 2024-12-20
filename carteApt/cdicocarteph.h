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
class cDicoCartepH;

int cleNH(const siglePedo *s, int AE, int SS);

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
        mPHASE_5=p->getPHASE_5();
        mPHASE_6=p->getPHASE_6();

        mCHARGE=p->getCHARGE();
        mDRAINAGE=p->getDRAINAGE();
        mDEV_PROFIL=p->getDEV_PROFIL();
        mSER_SPEC=p->getSER_SPEC();
        prepare();
    }

    // créer un sigle pédo depuis les relevés des placettes IPRFW suivi pédo
    siglePedo(std::string texture, std::string drainage,std::string profil, std::string charge, int profIPRFW, cDicoCartepH * dico);

    void cat() const{ std::cout << "sol index " << INDEX_ << " Substrat " << SUBSTRAT << " Texture " << mMAT_TEXT << " mPHASE_1 " << mPHASE_1 << " Charge " << mCHARGE << " drainage " << mDRAINAGE << " PROFIL " << mDEV_PROFIL << " MAT text simp "<< mMAT_TEXT_SIMP <<  std::endl;
                      std::cout << "mText_ZSP " <<mText_ZSP << " mText_LAEU " << mText_LAEU << " mTextG " << mTextG << " mText_ZSPenrichi " << mText_ZSPenrichi << std::endl;
                      std::cout << " tourbe " << tourbe() << ", profond " << profond() << ", mPHASE_2 " << mPHASE_2<<  std::endl;
                      std::cout << "mDb " << mDb << " mDa " << mDa << " mDc " << mDc << std::endl;
                    }

    void setMAT_TEXT_SIMP(std::string aMatSimp){mMAT_TEXT_SIMP=aMatSimp;}
    int getIndex() const {return INDEX_;}
    std::string getSUBSTRAT() const {return SUBSTRAT;}
    std::string getMAT_TEXT() const {return mMAT_TEXT;}
    std::string getPHASE_1() const {return mPHASE_1;}
    std::string getPHASE_2() const {return mPHASE_2;}
    std::string getPHASE_4() const {return mPHASE_4;}
     std::string getPHASE_5() const {return mPHASE_5;}
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
    bool calcaireLorraine() const {return mCalcaireLorraine;}
    bool fauxCalcaireLorraine() const {return mFauxCalcaireLorraine;}
    // serie.special.riche
    bool ssriche() const {return mSsriche;}
    bool profond() const{return mProfond;}
    bool alluvion() const{return mAlluvion;}
    bool podzol() const{return mPodzol;}
    bool podzolique() const{return mPodzolique;}
    bool superficiel() const{return mSuperficiel;}
    bool tourbe() const{return mTourbe;}
    bool argileBlanche() const{return mArgileBlanche;}
    bool limon() const{return mLimon;}
    bool chargeSchisteux() const{return mCHARGE=="f";}

    bool substratSchisteux() const{
        bool aRes(0);
        if ((SUBSTRAT.find("f")!=std::string::npos) && (SUBSTRAT.find("(")==std::string::npos) && (SUBSTRAT.find("k")==std::string::npos)){aRes=1;}
        return aRes;}

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
        dbo::field(a, mPHASE_5, "PHASE_5");
        dbo::field(a, mPHASE_6, "PHASE_6");
        dbo::field(a, mCHARGE,     "CHARGE");
        dbo::field(a, mDRAINAGE,    "DRAINAGE");
        dbo::field(a, mDEV_PROFIL,    "DEV_PROFIL");
        dbo::field(a, mSER_SPEC,    "SER_SPEC");
    }

    // pour NH
    bool mSsHumid,mSsSource,mSsRavin,mSsAffleurementRocheux,mSsAffleurementIterm,mSsPente,mSss,mProf2,mProf3,mProf45,mProf6;
    bool mDa,mDb,mDc,mDd,mDe,mDf,mDg,mDh,mDi;
    bool mText_ZSP,mText_LAEU,mTextG,mText_ZSPenrichi;
    std::string mMAT_TEXT_SIMP;
private:
    bool mCalcaire,mSsriche,mProfond,mAlluvion,mPodzol,mPodzolique,mSuperficiel,mTourbe,mLimon, mCalcaireLorraine,mFauxCalcaireLorraine,mArgileBlanche;
    std::string SUBSTRAT,mMAT_TEXT, mPHASE_1, mPHASE_2,mPHASE_4,mPHASE_5, mPHASE_6, mCHARGE, mDRAINAGE, mDEV_PROFIL, mSER_SPEC;
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

    std::string getMatTextSimp(std::string aText){
        std::string aRes="";
        if(Dico_MAT_TEXT.find(aText)!=Dico_MAT_TEXT.end()){aRes=Dico_MAT_TEXT.at(aText);}
        return aRes;
    }

    int getEpaisseur(int aSolIndex){
        int aRes(0);
        if (Dico_IndexSiglePed2Epaisseur.find(aSolIndex)!=Dico_IndexSiglePed2Epaisseur.end()){aRes=Dico_IndexSiglePed2Epaisseur.at(aSolIndex);}
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
            s2->setMAT_TEXT_SIMP(getMatTextSimp(s2->getMAT_TEXT()));
            // refaire prepare sinon les critères qui utilisent MAT_TEXT_SIMP ne vont pas fonctionner...
            s2->prepare();
            mMSigles.emplace(std::make_pair(aIndex,s2));
            aRes=s2;
        }

        return aRes;
    }

    /*siglePedo * getSiglePedoPtr(std::string aSigle){
        std::cout << "je veux charger le signe pedo " << aSigle << std::endl;
            dbo::Transaction transaction{session};
            dbo::ptr<siglePedo> s = session.find<siglePedo>().where("SIGLE_PEDO=?").bind(aSigle);
            siglePedo * s2=new siglePedo(s);
            s2->setMAT_TEXT_SIMP(getMatTextSimp(s2->getMAT_TEXT()));
            // refaire prepare sinon les critères qui utilisent MAT_TEXT_SIMP ne vont pas fonctionner...
            s2->prepare();
            return s2;
    }*/

private:
    std::string mBDpath;
    sqlite3 *db_;

    std::map<std::string,int> Dico_NomRN2Code;
    std::map<int,std::string> Dico_codeRN2Nom;
    std::map<int,std::string> Dico_PTS;
    std::map<int,int> Dico_Zbio2RN;
    std::map<int,int> Dico_IndexSiglePed2PTS;
    std::map<int,int> Dico_IndexSiglePed2Epaisseur;
    //std::map<int,int> Dico_Epaisseur2rastCode;
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
