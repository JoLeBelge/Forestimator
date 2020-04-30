#include "grouplayers.h"

//const char *cl[] = { "FEE", "CS" };
const TypeClassifST cl[] = { FEE, CS };
std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};

groupLayers::groupLayers(cDicoApt * aDico, WContainerWidget *parent, WContainerWidget *infoW, WOpenLayers *aMap, WApplication *app):mDico(aDico),mTypeClassifST(FEE),mInfoW(infoW),mMap(aMap),mParent(parent),mPBar(NULL),m_app(app)
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
    Layer aL(this,"IGN",label,TypeLayer::Externe);
    mVLs.push_back(aL);
    row++;
    if (row % 6 == 0){col++;row=0;}
    // creation des layers pour les KK du CS
    for (auto & pair : *mDico->codeKK2Nom()){
        WText *label;
        label = mOtherTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
        Layer aL(this,pair.first,label,TypeLayer::KK);
        mVLs.push_back(aL);
        row++;
        if (row % 6 == 0){col++;row=0;}
    }
    // ajout des cartes "FEE" ; NT NH Topo AE SS
    for (auto & pair : *mDico->RasterType()){
        WText *label;
        label = mOtherTable->elementAt(row,col)->addWidget(cpp14::make_unique<WText>(""));
        Layer aL(this,pair.first,label,TypeLayer::Thematique);
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
            //Layer aL(this,pair.first,label);
            // constructeur pour type Apti pour ne pas créer deux fois l'essence d'affilé, ouverture et fermeture de la DB un peu sensible
            Layer aL(this,ess,label);
            mVLs.push_back(aL);
            row++;
            if (row % 17 == 0){col++;row=0;}
        }
    }
    // création de la légende (vide pour le moment)
    mLegend = new legend(this,infoW);
    mStation = new ST(mDico);

    // création des arbres pour sélection des couches - ces objets sont affiché ailleur
    mSelect4Stat= new selectLayers4Stat(this);
    mSelect4Download= new selectLayers4Download(this);
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


    mStation->vider();
    mLegend->vider();

    // tableau des informations globales - durant ce round, l'objet ST est modifié
    mLegend->titreInfoRaster();

    for (Layer& l : mVLs){
        //if (l.IsActive()) l.displayInfo(x,y,mInfoW);
        if ((l.Type()==TypeLayer::KK )| (l.Type()==TypeLayer::Thematique )|( l.IsActive())){
            mLegend->add1InfoRaster(l.displayInfo(x,y));
        }
    }

    // tableau du détail du calcul de l'aptitude d'une essence pour FEE
    for (Layer& l : mVLs){
        // on a bien une essence active et on est en mode FEE
        if ( l.IsActive() && l.Type()==TypeLayer::Apti && mTypeClassifST==FEE){
            mLegend->detailCalculAptFEE(mStation);
        }
    }
    // tableau des aptitudes pour toutes les essences
    mLegend->afficheAptAllEss();

    Wt::WAnimation animation(Wt::AnimationEffect::Fade,
                             Wt::TimingFunction::Linear,
                             1000);

    mLegend->animateShow(animation); // marche pas...
    mMap->animateShow(animation);

    // je voulais faire une animation du curseur, echec. mais j'utiliser les animationShow maintenant
    //usleep(10000);
    //mMap->decorationStyle().setCursor(Cursor::Arrow);
    //mParent->decorationStyle().setCursor(Cursor::Auto);
    //mParent->decorationStyle().set
}


// clé 1 ; nom de la couche. clé2 : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
std::map<std::string,std::map<std::string,int>> groupLayers::computeStatGlob(OGRGeometry *poGeomGlobale){
    std::cout << " groupLayers::computeStatGlob " << std::endl;
    // clear d'un vecteur de pointeur, c'est mal.
    for (auto p : mVLStat)
       {
         delete p;
       }
    mVLStat.clear();
    std::map<std::string,std::map<std::string,int>> aRes;

    // il faut filtrer sur les couches sélectionnées et gerer un changement de mode FEE/CS pour les layers aptitude.
    for (auto &kv: getSelectedLayer4Stat() ){
        Layer * l=kv.second;
        std::string aMode=kv.first.at(1);

        // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        std::map<std::string,int> stat = l->computeStatOnPolyg(poGeomGlobale,aMode);

        // c'est parcellaire:: qui doit gerer l'affichage des layerStatChart
        layerStatChart* aLayStatChart=new layerStatChart(l,stat,aMode);
        mVLStat.emplace_back(aLayStatChart);

        aRes.emplace(std::make_pair(l->getCode(),stat));
        mPBar->setValue(mPBar->value() + 1);
        m_app->processEvents();
    }
    mPBar->setValue(mPBar->maximum());
    std::cout << " done " << std::endl;
    return aRes;
}

void groupLayers::computeStatOnPolyg(OGRLayer * lay,bool mergeOT){

    for (auto &kv: getSelectedLayer4Stat() ){
        Layer * l=kv.second;
        std::string aMode=kv.first.at(1);

        // défini le nouveau champ à ajouter à la table d'attribut - vérifie qu'il n'existe pas préhalablement
        if (lay->FindFieldIndex(l->getCode().c_str(),0)==-1){
            OGRFieldDefn oFLD(l->getCode().c_str(),  OFTInteger);
            oFLD.SetJustify(OGRJustification::OJLeft);
            //oFLD.SetDefault(0);
            lay->CreateField(&oFLD);
        }

        OGRFeature *poFeature;
        lay->ResetReading();
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
            OGRGeometry * poGeom = poFeature->GetGeometryRef();
            poGeom->flattenTo2D();
            //std::map<std::string,int> stat = l->computeStatOnPolyg(poGeom,aMode);
            layerStat ls(l,l->computeStatOnPolyg(poGeom,aMode),aMode);
            // on met un résumé des stat dans le champ nouvellement créé
            poFeature->SetField(l->getCode().c_str(), ls.getO(mergeOT));
            //poFeature->SetField(); This method has only an effect on the in-memory feature object. If this object comes from a layer and the modifications must be serialized back to the datasource, OGR_L_SetFeature()
            lay->SetFeature(poFeature);
        }
    }
}

std::map<std::string,int> groupLayers::apts(){
    std::map<std::string,int> aRes;
    switch (mTypeClassifST){
    case FEE:
        if (mStation->readyFEE()){
            for (Layer l : mVLs){
                if (l.Type()==TypeLayer::Apti){
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
                if (l.Type()==TypeLayer::Apti){
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

selectLayers4Stat::selectLayers4Stat(groupLayers * aGL):mGL(aGL),selectLayers(aGL,aGL->getVpLs(),10){
    std::cout << "creation de selectLayers4Stat " << std::endl;

    // pour l'instant ; uniquement les aptitudes FEE
    for (Layer * l : mVpLs){
        if (l->Type()!=TypeLayer::Externe){
            if (l->Type()==TypeLayer::Apti){
                std::vector<std::string> aKey2={l->getCode(),"FEE"};
                bool b(0);
                if (l->getCode()=="HE"|| l->getCode()=="CS" || l->getCode()=="CP" || l->getCode()=="EP" || l->getCode()=="DO" || l->getCode()=="ME"){b=true;}
                mSelectedLayers.emplace(std::make_pair(aKey2,b));
                mLayersCBox.emplace(std::make_pair(aKey2,new Wt::WCheckBox()));
            }
        }
    }

    cont = new Wt::WContainerWidget();
    cont->setOverflow(Wt::Overflow::Auto);
    treeTable = cont->addWidget(cpp14::make_unique<WTreeTable>());
    treeTable->resize(300, 250);
    treeTable->setStyleClass("tree");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox
    auto root = cpp14::make_unique<WTreeTableNode>("Carte d'aptitudes FEE");
    treeTable->setTreeRoot(std::move(root), "Raster");

    for (auto kv : mSelectedLayers){
        // une méthode pour récupérer un ptr vers le layer depuis le code aCode+aMode
        Layer * l = getLayerPtr(kv.first);
        std::string aCode(l->getCode());
        auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
        auto node1_ = node1.get();
        treeTable->treeRoot()->addChildNode(std::move(node1));
        WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,"FEE"});
        if (isSelected(l->getCode(),"FEE")){check1_->setChecked();}
        check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode,"FEE");});
        node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
    }
    treeTable->treeRoot()->expand();
}

Layer* selectLayers::getLayerPtr(std::vector<std::string> aCode){
    Layer*  aRes=NULL;
    for (Layer * l : mVpLs){
        if (l->getCode()==aCode.at(0)){aRes=l;}
    }
    return aRes;
}

std::map<std::vector<std::string>,Layer*> selectLayers::getSelectedLayer(){
    std::map<std::vector<std::string>,Layer*> aRes;
    for (auto kv : mSelectedLayers){
        if (kv.second){
            aRes.emplace(std::make_pair(kv.first,getLayerPtr(kv.first)));
        }
    }
    return aRes;
}

selectLayers4Download::selectLayers4Download(groupLayers * aGL):mGL(aGL),selectLayers(aGL,aGL->getVpLs(),75){
    std::cout << "creation de selectLayers4Download " << std::endl;
    for (Layer * l : mVpLs){
        if (l->Type()!=TypeLayer::Externe){
            if (l->Type()==TypeLayer::Apti){
                std::vector<std::string> aKey1={l->getCode(),"CS"};
                std::vector<std::string> aKey2={l->getCode(),"FEE"};
                // default ; on ne veux pas les apt CS
                mSelectedLayers.emplace(std::make_pair(aKey1,false));
                mSelectedLayers.emplace(std::make_pair(aKey2,true));
                mLayersCBox.emplace(std::make_pair(aKey1,new Wt::WCheckBox()));
                mLayersCBox.emplace(std::make_pair(aKey2,new Wt::WCheckBox()));
            } else {
                std::vector<std::string> aKey1={l->getCode(),""};
                mSelectedLayers.emplace(std::make_pair(aKey1,false));
                mLayersCBox.emplace(std::make_pair(aKey1,new Wt::WCheckBox()));
            }
        }
    }

    cont = new Wt::WContainerWidget();
    cont->setOverflow(Wt::Overflow::Auto);
    treeTable = cont->addWidget(cpp14::make_unique<WTreeTable>());
    treeTable->resize(300, 500);
    treeTable->setStyleClass("tree");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox
    auto root = cpp14::make_unique<WTreeTableNode>("Tous");
    treeTable->setTreeRoot(std::move(root), "Raster");

    // création des groupes de couches avec checkbox qui permet de toutes les selectionner en un click
    // aptitude FEE
    auto grAptFEE = cpp14::make_unique<WTreeTableNode>("Aptitudes FEE");
    WTreeTableNode *grAptFEE_ = grAptFEE.get();
    std::unique_ptr<WCheckBox> checkAptFEE = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptFEE_ = checkAptFEE.get();
    checkAptFEE_->changed().connect([=]{SelectLayerGroup(checkAptFEE_->isChecked(),TypeLayer::Apti,"FEE");});
    grAptFEE->setColumnWidget(1, std::move(checkAptFEE));

    // aptitude CS
    auto grAptCS = cpp14::make_unique<WTreeTableNode>("Aptitudes CS");
    WTreeTableNode *grAptCS_ = grAptCS.get();
    std::unique_ptr<WCheckBox> checkAptCS = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptCS_ = checkAptCS.get();
    checkAptCS_->changed().connect([=]{SelectLayerGroup(checkAptCS_->isChecked(),TypeLayer::Apti,"CS");});
    grAptCS->setColumnWidget(1, std::move(checkAptCS));

    // diagnostic Stationnel
    auto grSt = cpp14::make_unique<WTreeTableNode>("Cartes diagnostic stationnel");
    WTreeTableNode *grSt_ = grSt.get();
    std::unique_ptr<WCheckBox> checkSt = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkSt_ = checkSt.get();
    checkAptCS_->changed().connect([=]{SelectLayerGroup(checkSt_->isChecked(),TypeLayer::Thematique,"");});
    grSt->setColumnWidget(1, std::move(checkSt));

    // habitats, potentiel sylvicole
    auto grKK = cpp14::make_unique<WTreeTableNode>("Habitats et potentiel sylvicole");
    WTreeTableNode *grKK_ = grKK.get();
    std::unique_ptr<WCheckBox> checkKK = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkKK_ = checkKK.get();
    checkKK_->changed().connect([=]{SelectLayerGroup(checkKK_->isChecked(),TypeLayer::KK,"");});
    grKK->setColumnWidget(1, std::move(checkKK));

    for (Layer * l : mVpLs){
        std::string aCode=l->getCode();
        switch (l->Type()){
        case TypeLayer::Apti:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grAptFEE_->addChildNode(std::move(node1));
            //std::unique_ptr<WCheckBox> check1 = cpp14::make_unique<WCheckBox>();
            //WCheckBox * check1_ = check1.get();
            WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,"FEE"});
            if (isSelected(l->getCode(),"FEE")){check1_->setChecked();}
            // avec bind, check is checked ; est toujours à false.
            //check->changed().connect(std::bind(&groupLayers::SelectLayer,this,check->isChecked(),l.getCode(),"FEE"));
            check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode,"FEE");});
            node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            auto node2 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node2_ = node2.get();
            grAptCS_->addChildNode(std::move(node2));
            //std::unique_ptr<WCheckBox> check2 = cpp14::make_unique<WCheckBox>();
            //if (isChecked(l.getCode(),"CS")){check2_->setChecked();}
            //WCheckBox * check2_ = check2.get();
            WCheckBox * check2_ = mLayersCBox.at(std::vector<std::string> {aCode,"CS"});
            check2_->changed().connect([=]{SelectLayer(check2_->isChecked(),aCode,"CS");});
            //node2_->setColumnWidget(1, std::move(check2));
            node2_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check2_));
            break;
        }
        case TypeLayer::Thematique:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grSt_->addChildNode(std::move(node1));
            WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,""});
            if (isSelected(l->getCode())){check1_->setChecked();}
            check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode);});
            node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            break;
        }
        case TypeLayer::KK:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grKK_->addChildNode(std::move(node1));
            WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,""});
            if (isSelected(l->getCode())){check1_->setChecked();}
            check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode);});
            node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            break;
        }
        default:{

        }
        }
    }

    treeTable->treeRoot()->addChildNode(std::move(grAptFEE));
    treeTable->treeRoot()->addChildNode(std::move(grAptCS));
    treeTable->treeRoot()->addChildNode(std::move(grSt));
    treeTable->treeRoot()->addChildNode(std::move(grKK));
    treeTable->treeRoot()->expand();
}

void selectLayers::SelectLayer(bool select, std::string aCode, std::string aMode, bool afficheMsg){

    std::vector<std::string> aKey={aCode,aMode};
    if ((!select) |(nbMax>(numSelectedLayer()))){
        if (mSelectedLayers.find(aKey)!=mSelectedLayers.end()){
            mSelectedLayers.at(aKey)=select;
            mLayersCBox.at(aKey)->setChecked(select);
        } else {
            std::cout << "je ne trouve pas la couche qui a le code " << aCode << " et le mode " << aMode << std::endl;
        }
    } else {
        // faire apparaitre un message à l'utilisateur, maximum nbMax
        // changer le status de la checkbox, remettre à false
        if (select) mLayersCBox.at(aKey)->setChecked(false);
        if (afficheMsg){
        auto messageBox =
                mParent->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                      "Sélection des couches",
                                      "<p>Vous avez atteint le maximun de " + std::to_string(nbMax)+ " couches</p>"
                                                                                                     "<p>Veillez déselectionner une couche avant d'en sélectionner une nouvelle</p>",
                                      Wt::Icon::Information,
                                      Wt::StandardButton::Ok));

        messageBox->setModal(false);
        messageBox->buttonClicked().connect([=] {
            mParent->removeChild(messageBox);
        });
        messageBox->show();
        }

    }

    std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;
    // pourrait envoyer un signal au widget upload pour transmettre le nombre de couches sélectionnées pour affichage
}

void selectLayers::SelectLayerGroup(bool select,TypeLayer aType,std::string aMode){
    for (Layer * l : mVpLs){
        if (l->Type()==aType){
            SelectLayer(select,l->getCode(),aMode,false);
        }
    }
    std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;
}


// pour envoyer la liste des raster à uploadcarte
std::vector<rasterFiles> selectLayers::getSelectedRaster(){
    std::vector<rasterFiles> aRes;
    for (Layer * l : mVpLs){
        switch (l->Type()){
        case TypeLayer::Apti:{
            if (isSelected(l->getCode(),"FEE")) aRes.push_back(rasterFiles(l->getPathTif("FEE")));
            if (isSelected(l->getCode(),"CS")) aRes.push_back(rasterFiles(l->getPathTif("CS")));
            break;
        }
        default:{
            if (isSelected(l->getCode())) aRes.push_back(rasterFiles(l->getPathTif()));
            break;
        }
        }
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


