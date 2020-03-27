#include "grouplayers.h"

//const char *cl[] = { "FEE", "CS" };
const TypeClassifST cl[] = { FEE, CS };
std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};

groupLayers::groupLayers(cDicoApt * aDico, WContainerWidget *parent, WContainerWidget *infoW):mDico(aDico),mTypeClassifST(FEE),mInfoW(infoW)
{
    setOverflow(Wt::Overflow::Auto);
    setPadding(20);
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    this->addStyleClass("table form-inline");
    this->setStyleClass("table form-inline");
    // creation de la table listant les cartes thématiques - catalogue station,pot sylvicole, NH, NT, AE, ect
    mOtherTable = addWidget(cpp14::make_unique<Wt::WTable>());
    int row(0),col(0);

    mOtherTable->setWidth("80%");

    // carte IGN
    WText *label;
    label = mOtherTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
    Layer aL(this,"IGN",label,Externe);
    mVLs.push_back(aL);
    row++;
    if (row % 6 == 0){col++;row=0;}
    // creation des layers pour les KK du CS
    for (auto & pair : *mDico->codeKK2Nom()){
        WText *label;
        label = mOtherTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
        Layer aL(this,pair.first,label,KK);
        mVLs.push_back(aL);
        row++;
        if (row % 6 == 0){col++;row=0;}
    }
    // ajout des cartes "FEE" ; NT NH Topo AE SS
    for (auto & pair : *mDico->RasterType()){
        WText *label;
        label = mOtherTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
        Layer aL(this,pair.first,label,Thematique);
        mVLs.push_back(aL);
        row++;
        if (row % 6 == 0){col++;row=0;}
    }

    mClassifTable = addWidget(cpp14::make_unique<Wt::WTable>());
    for (int i = 0; i < 2; ++i) {
        WText *t = mClassifTable->elementAt(0,i)->addWidget(cpp14::make_unique<WText>(classes[i]));
        mClassifTable->elementAt(0,i)->setContentAlignment(AlignmentFlag::Center| AlignmentFlag::Middle);
        t->setMargin(5);
        t->clicked().connect(std::bind(&groupLayers::changeClassClick, this, t));
        bool isCurrent = i==0;
        //choisi un des deux style
        t->setStyleClass(isCurrent ? "currentEss" : "ess");
        clasLabels_.push_back(t);
    }
    //mClassifTable->setOffsets(100);
    mClassifTable->setHeight(75); // ça plus le setContentAlig

    // creation de la table listant les essences
    mEssTable = addWidget(cpp14::make_unique<Wt::WTable>());
    row=0;col=0;

    mEssTable->setWidth("80%");

    // creation des layers pour les essences qui ont des aptitudes
    for (auto & pair : *mDico->code2Nom()){
        cEss ess(pair.first,mDico);
        if (ess.hasApt()){
            WText *label;
            label = mEssTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
            Layer aL(this,pair.first,label);
            mVLs.push_back(aL);
            row++;
            if (row % 17 == 0){col++;row=0;}
        }
    }

    // création de la légende (vide pour le moment)
    mLegend = new legend(this,infoW);
    mStation = new ST(mDico);
}

void groupLayers::update(std::string aCode){
    //std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    // désactiver toutes les couches actives et changer le rendu du label
    for (Layer & l : mVLs){
        l.setActive(aCode==l.getCode());
    }
}

void groupLayers::clickOnName(std::string aCode){

    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    update(aCode);

    // ajouter la couche à la carte
    for (Layer& l : mVLs){
        if (l.IsActive()){
            l.displayLayer();
            mLegend->afficheLegendeIndiv(&l);
        }
    }


    // donner le focus à la carte en envoyant un signal. mais ce n'est pas ça que je veux moi.
    //focusOnMap_.emit(true);
}

void groupLayers::changeClassClick(WText *t)
{

    //std::cout << "change classif station, actuellement " << currentClassifST << ", on veux mettre " << t->text().toUTF8() << "\n" << std::endl;
    int index(0);
    for (auto i : clasLabels_) {
        // ouch vicieux ça
        WText *l = i;
        //std::cout <<"   l  est pointeur vers " << i->text().toUTF8() << std::endl;
        // prefix match, e.g. en matches en-us.
        bool isCurrent = t->text().toUTF8()==l->text().toUTF8();
        if (isCurrent) { mTypeClassifST = cl[index];}
        //choisi un des deux style
        l->setStyleClass(isCurrent ? "currentEss" : "ess");
        index++;
    }

    // ajouter la couche à la carte
    for (Layer& l : mVLs){
        if (l.IsActive()) l.displayLayer();
    }
}

void groupLayers::extractInfo(double x, double y){
    // vider la fenetre info. le clear() ne suffit pas
    /*for (auto & w : mInfoW->children()){
    mInfoW->removeChild(w);
    }*/
    mStation->vider();
    mLegend->vider();

    // tableau des informations globales - durant ce round, l'objet ST est modifié
    mLegend->titreInfoRaster();

    for (Layer& l : mVLs){
        //if (l.IsActive()) l.displayInfo(x,y,mInfoW);
        if (l.Type()==KK | l.Type()==Thematique | l.IsActive()){
        mLegend->add1InfoRaster(l.displayInfo(x,y));
        }
    }

    // tableau du détail du calcul de l'aptitude d'une essence pour FEE
    for (Layer& l : mVLs){
        // on a bien une essence active et on est en mode FEE
        if ( l.IsActive() && l.Type()==Apt && mTypeClassifST==FEE){
        mLegend->detailCalculAptFEE(mStation);
        }
    }
    // tableau des aptitudes pour toutes les essences
    mLegend->afficheAptAllEss();

}

std::map<std::string,int> groupLayers::apts(){
    std::map<std::string,int> aRes;
    switch (mTypeClassifST){
    case FEE:
    if (mStation->readyFEE()){
    for (Layer l : mVLs){
        if (l.Type()==Apt){
        // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
       cEss  * Ess= l.Ess();
       int apt = Ess->getFinalApt(mStation->mNT,mStation->mNH, mStation->mZBIO, mStation->mTOPO);
        aRes.emplace(std::make_pair(Ess->Code(),apt));
        }
    }
    }
        break;
    case CS:
        if (mStation->readyCS()){
        for (Layer l : mVLs){
            if (l.Type()==Apt){
            // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
           cEss  * Ess= l.Ess();
           int apt = Ess->getApt(mStation->mZBIO, mStation->mSt);
           if (apt!=0) aRes.emplace(std::make_pair(Ess->Code(),apt));
           }
        }
        }
       break;
    }
    return aRes;
}

ST::ST(cDicoApt * aDico):mDico(aDico),mNT(666),mNH(666),mZBIO(666),mTOPO(666),mActiveEss(0),HaveEss(0),mSt(0)
{

}

void ST::vider()
{
mNT=666;
mNH=666;
mZBIO=666;
mTOPO=666;
mActiveEss=0;
HaveEss=0;
mSt=666;
}
