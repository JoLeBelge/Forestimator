#include "grouplayers.h"

const TypeClassifST cl[] = { FEE, CS };
std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};

groupLayers::groupLayers(cDicoApt * aDico, WOpenLayers *aMap, WApplication *app, stackInfoPtr * aStackInfoPtr):
    mDico(aDico)
  ,mTypeClassifST(FEE)
  ,mMap(aMap)
  ,mParent(aStackInfoPtr->mGroupLayerW)
  ,mPBar(NULL)
  ,m_app(app)
  ,mLegend(NULL)
  ,mSelect4Stat(NULL)
  ,mSelect4Download(NULL)
  ,mStackInfoPtr(aStackInfoPtr)
  ,mapExtent_(this,"1.0")
{
    //std::cout << "constructeur GL " << std::endl;
    mParent->setOverflow(Wt::Overflow::Visible);
    mParent->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
    mParent->addWidget(cpp14::make_unique<WText>( tr("coucheStep1")));

    slot.setJavaScript("function toto(e) {"
                       "var extent = map.getView().calculateExtent(map.getSize());"
                       "var bottomLeft = ol.extent.getBottomLeft(extent);"
                       "var topRight =ol.extent.getTopRight(extent);"
                       "if (bottomLeft != null) {"
                       + mapExtent_.createCall({"topRight[0]","topRight[1]","bottomLeft[0]","bottomLeft[1]"})
                       + "}}"
                       );
    // on exécute une première fois le script js pour initialiser l'enveloppe mapExtent
    //slot.exec();plus nécessaire
    this->getMapExtendSignal().connect(std::bind(&groupLayers::updateMapExtentAndCropIm,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));

    updateGL();
    // TODO on click event

    /*   AUTRES ONLGETS de la stack   */
    // création de la légende (vide pour le moment)
    mLegend = new legend(this, mStackInfoPtr->mLegendW);
    mStation = new ST(mDico);

    //std::cout << "done" << std::endl;
}

/*groupLayers::~groupLayers(){
    std::cout << "destructeur de group layer " << std::endl;
    delete mLegend;
    delete mStation;
    delete mSelect4Stat;
    delete mSelect4Download;
    printf("null mMap GL\n");
    mMap=NULL;
    m_app=NULL;
    mDico=NULL;
    mLegend=NULL;
    printf("nulled mMap GL\n");
    mPBar=NULL;
    mVLs.clear();
}*/

void groupLayers::update(std::string aCode, TypeLayer type){
    //std::cout << " group Layers je vais faire un update du rendu visuel de chacun des label de couche \n\n\n" << std::endl;
    // désactiver toutes les couches actives et changer le rendu du label
    for (Layer * l : mVLs){
        l->setActive(aCode==l->getCode() && type==l->Type());
    }
    //std::cout << "update done " << std::endl;
}

void groupLayers::clickOnName(std::string aCode, TypeLayer type){

    //std::cout << " j'ai cliqué sur un label " << aCode <<  "\n\n"<< std::endl;
    // udpate du rendu visuel de tout les labels de couches -- cela se situe au niveau du grouplayer
    update(aCode, type);

    // ajouter la couche à la carte
    for (Layer * l : mVLs){
        if (l->IsActive() && type==l->Type()){
            l->displayLayer();
            mLegend->afficheLegendeIndiv(l);
            break;
        }
    }
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
    for (Layer * l : mVLs){
        if (l->IsActive()) l->displayLayer();
    }
}

void groupLayers::extractInfo(double x, double y){

    if(!isnan(x) && !isnan(y) && !(x==0 && y==0)){
    std::cout << "groupLayers ; extractInfo " << x << " , " << y << std::endl;
    mStation->vider();
    mLegend->vider();

    // maintenant on n'affiche plus la légende automatiquement, , pL n'aime pas
    //mStackInfoPtr->menuitem2_legend->select();
    //mStackInfoPtr->stack_info->setCurrentIndex(0);

    // tableau des informations globales - durant ce round, l'objet ST est modifié
    mLegend->titreInfoRaster();

    for (Layer * l : mVLs){
        if (((l->Type()==TypeLayer::KK )| (l->Type()==TypeLayer::Thematique )) | (( l->IsActive()) & (l->Type()!=TypeLayer::Externe))){

            std::vector<std::string> layerLabelAndValue=l->displayInfo(x,y);
            mLegend->add1InfoRaster(layerLabelAndValue);

            if (( l->IsActive())){
                // affiche une popup pour indiquer la valeur pour cette couche
                // attention, il faut escaper les caractères à problèmes du genre apostrophe
                boost::replace_all(layerLabelAndValue.at(1),"'","\\'"); // javascript bug si jamais l'apostrophe n'est pas escapée
                boost::replace_all(layerLabelAndValue.at(0),"'","\\'");
                mParent->doJavaScript("content.innerHTML = '<p>"+layerLabelAndValue.at(0)+":</p><code>"+ layerLabelAndValue.at(1)+ "</code>';"
                                      +"var coordinate = ["+std::to_string(x) + ","+ std::to_string(y) +"];"
                                      +"overlay.setPosition(coordinate);"
                                      +"overlay.setPosition(coordinate);"
                                      );
            }

        }
    }

    // tableau du détail du calcul de l'aptitude d'une essence pour FEE
    for (Layer * l : mVLs){
        // on a bien une essence active et on est en mode FEE
        if ( l->IsActive() && l->Type()==TypeLayer::FEE && mTypeClassifST==FEE){
            mLegend->detailCalculAptFEE(mStation);
        }
    }
    // tableau des aptitudes pour toutes les essences
    mLegend->afficheAptAllEss();

   /* Wt::WAnimation animation(Wt::AnimationEffect::Fade,
                             Wt::TimingFunction::Linear,
                             1000);

    // si je double clique très vite deux fois, l'animation numéro deux sera reportée à plus tard (buggy)
    //mLegend->animateShow(animation); // marche pas... car mLegend est une classe qui hérite d'un conteneur mais ce conteneur n'est pas affiché, tout les objets sont dans le conteneur appelé à tord "Parent"
    //mLegend->mParent->resize("100%","100%");// j'ai changé le contenu du parent
    mLegend->mParent->animateShow(animation);
    */


    mMap->updateView();
    }else {
        std::cout << "x et y ne sont pas des nombres , pas bien " << std::endl;
    }
}

// clé 1 ; nom de la couche. clé2 : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
void groupLayers::computeStatGlob(OGRGeometry *poGeomGlobale){
    std::cout << " groupLayers::computeStatGlob " << std::endl;
    // clear d'un vecteur de pointeur, c'est mal.
    for (auto p : mVLStat)
    {
        delete p;
    }
    mVLStat.clear();
    //std::map<std::string,std::map<std::string,int>> aRes;

    // il faut filtrer sur les couches sélectionnées et gerer un changement de mode FEE/CS pour les layers aptitude.
    // non, pour les statistiques globales, on prend toutes les couches - bof trop long et surtout, trop de camemberts - je prends celles pour download
    //for (auto &kv: getAllLayer() ){
    for (auto &kv: getSelectedLayer4Download() ){
        Layer * l=kv.second;
        std::string aMode=kv.first.at(1);
        // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        std::map<std::string,int> stat = l->computeStatOnPolyg(poGeomGlobale,aMode);
        // c'est parcellaire:: qui doit gerer l'affichage des layerStatChart
        //layerStatChart* aLayStatChart=new layerStatChart(l,stat,aMode);
        //mVLStat.emplace_back(aLayStatChart);
        mVLStat.push_back(new layerStatChart(l,stat,aMode));
        //aRes.emplace(std::make_pair(l->getCode(),stat));
        mPBar->setValue(mPBar->value() + 1);
        m_app->processEvents();
    }
    mPBar->setValue(mPBar->maximum());
    std::cout << " done " << std::endl;
    //return aRes;
}

void groupLayers::computeStatOnPolyg(OGRLayer * lay,bool mergeOT){

    for (auto &kv: getSelectedLayer4Stat() ){
        Layer * l=kv.second;
        std::string aMode=kv.first.at(1);

        // défini le nouveau champ à ajouter à la table d'attribut - vérifie qu'il n'existe pas préhalablement
        if (lay->FindFieldIndex(l->getFieldName().c_str(),0)==-1){

            OGRFieldDefn * oFLD(NULL);
            if (l->getFieldType()=="int"){
                oFLD= new OGRFieldDefn(l->getFieldName().c_str(),  OFTInteger);
            }
            if (l->getFieldType()=="str"){
                oFLD= new OGRFieldDefn(l->getFieldName().c_str(),  OFTString);
            }

            oFLD->SetJustify(OGRJustification::OJLeft);

            lay->CreateField(oFLD);
        }

        OGRFeature *poFeature;
        lay->ResetReading();
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            // clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
            OGRGeometry * poGeom = poFeature->GetGeometryRef();
            poGeom->closeRings();
            poGeom->flattenTo2D();
            //std::map<std::string,int> stat = l->computeStatOnPolyg(poGeom,aMode);
            layerStat ls(l,l->computeStatOnPolyg(poGeom,aMode),aMode);
            // on met un résumé des stat dans le champ nouvellement créé
            if (l->getFieldType()=="int"){
                poFeature->SetField(l->getFieldName().c_str(), ls.getFieldVal(mergeOT));
            }
            if (l->getFieldType()=="str"){

                std::cout << "set field "<< l->getFieldName() << " to " <<    ls.getFieldValStr() << std::endl;
                poFeature->SetField(l->getFieldName().c_str(), ls.getFieldValStr().c_str());
            }

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
            for (Layer * l : mVLs){
                if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    cEss  * Ess= l->Ess();
                    int apt = Ess->getFinalApt(mStation->mNT,mStation->mNH, mStation->mZBIO, mStation->mTOPO);
                    aRes.emplace(std::make_pair(Ess->Code(),apt));
                }
            }
        }
        break;
    case CS:
        if (mStation->readyCS()){
            for (Layer * l : mVLs){
                if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){
                    // j'ai deux solution pour avoir les aptitudes ; soit je lis la valeur du raster apt, soit je recalcule l'aptitude avec les variables environnementales
                    cEss  * Ess= l->Ess();
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
    //std::cout << "creation de selectLayers4Stat " << std::endl;

    // pour l'instant ; uniquement les aptitudes FEE
    for (Layer * l : mVpLs){
        if (l->Type()!=TypeLayer::Externe){
            if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){
                std::vector<std::string> aKey2={l->getCode(),"FEE"};
                bool b(0);
                //printf("getcode: %s\n",l->getCode().c_str());
                if (l->getCode()=="HE"|| l->getCode()=="CS" || l->getCode()=="CP" || l->getCode()=="EP" || l->getCode()=="DO" || l->getCode()=="ME"){b=true;}
                mSelectedLayers.emplace(std::make_pair(aKey2,b));
                mLayersCBox.emplace(std::make_pair(aKey2,new Wt::WCheckBox()));
            }

            if (l->Type()==TypeLayer::Peuplement){
                std::vector<std::string> aKey2={l->getCode(),""};
                //printf("getcodeP : %s\n",l->getCode().c_str());
                mSelectedLayers.emplace(std::make_pair(aKey2,true));
                mLayersCBox.emplace(std::make_pair(aKey2,new Wt::WCheckBox()));
            }

        }
    }

    cont = new Wt::WContainerWidget();
    cont->setOverflow(Wt::Overflow::Auto);
    treeTable = cont->addWidget(cpp14::make_unique<WTreeTable>());
    //treeTable->resize(300, 250);
    treeTable->setHeight(241);
    treeTable->setStyleClass("tree");
    treeTable->addStyleClass("tree_left");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox

    auto root = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAll"));
    treeTable->setTreeRoot(std::move(root), "Couches");

    // création des groupes de couches avec checkbox qui permet de toutes les selectionner en un click
    // aptitude FEE
    auto grAptFEE = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAptFEE"));
    WTreeTableNode *grAptFEE_ = grAptFEE.get();
    std::unique_ptr<WCheckBox> checkAptFEE = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptFEE_ = checkAptFEE.get();
    checkAptFEE_->changed().connect([=]{SelectLayerGroup(checkAptFEE_->isChecked(),TypeLayer::FEE,"FEE");});
    grAptFEE->setColumnWidget(1, std::move(checkAptFEE));

    // aptitude CS
    /*
    auto grAptCS = cpp14::make_unique<WTreeTableNode>("Aptitudes CS");
    WTreeTableNode *grAptCS_ = grAptCS.get();
    std::unique_ptr<WCheckBox> checkAptCS = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptCS_ = checkAptCS.get();
    checkAptCS_->changed().connect([=]{SelectLayerGroup(checkAptCS_->isChecked(),TypeLayer::Apti,"CS");});
    grAptCS->setColumnWidget(1, std::move(checkAptCS));
    */

    // description du peuplement
    auto grPeup = cpp14::make_unique<WTreeTableNode>(tr("groupeCouchePeup"));
    WTreeTableNode *grPeup_ = grPeup.get();
    std::unique_ptr<WCheckBox> checkPeup = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkPeup_ = checkPeup.get();
    checkPeup_->changed().connect([=]{SelectLayerGroup(checkPeup_->isChecked(),TypeLayer::Peuplement,"");});
    grPeup->setColumnWidget(1, std::move(checkPeup));

    for (auto kv : mSelectedLayers){
        // une méthode pour récupérer un ptr vers le layer depuis le code aCode+aMode
        Layer * l = getLayerPtr(kv.first);
        std::string aCode(l->getCode());

        switch (l->Type()){
        case TypeLayer::CS:
        case TypeLayer::FEE:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grAptFEE_->addChildNode(std::move(node1));
            WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,"FEE"});
            if (isSelected(l->getCode(),"FEE")){check1_->setChecked();}
            check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode,"FEE");});
            node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            break;
        }
        case TypeLayer::Peuplement:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grPeup_->addChildNode(std::move(node1));
            WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,""});
            if (isSelected(l->getCode())){check1_->setChecked();}
            check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode);});
            node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            break;
        }
        default:{
            break;
        }
        }
    }

    treeTable->treeRoot()->addChildNode(std::move(grAptFEE));
    //treeTable->treeRoot()->addChildNode(std::move(grAptCS));
    treeTable->treeRoot()->addChildNode(std::move(grPeup));
    treeTable->treeRoot()->expand();
    //printf("4stat done");
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

std::map<std::vector<std::string>,Layer*> selectLayers::getAllLayer(){
    std::map<std::vector<std::string>,Layer*> aRes;
    for (auto kv : mSelectedLayers){
        aRes.emplace(std::make_pair(kv.first,getLayerPtr(kv.first)));
    }
    return aRes;
}

selectLayers4Download::selectLayers4Download(groupLayers * aGL):mGL(aGL),selectLayers(aGL,aGL->getVpLs(),25){
    //std::cout << "creation de selectLayers4Download " << std::endl;
    for (Layer * l : mVpLs){
        if (l->Type()!=TypeLayer::Externe){
            if (l->Type()==TypeLayer::FEE || l->Type()==TypeLayer::CS){


                std::vector<std::string> aKey2={l->getCode(),"FEE"};

                //if (ModeExpert){
                std::vector<std::string> aKey1={l->getCode(),"CS"};
                // default ; on ne veux pas les apt CS
                mSelectedLayers.emplace(std::make_pair(aKey1,false));
                mLayersCBox.emplace(std::make_pair(aKey1,new Wt::WCheckBox()));
                //}
                mSelectedLayers.emplace(std::make_pair(aKey2,false));
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
    //treeTable->resize(300, 250);
    treeTable->setHeight(241);
    treeTable->setStyleClass("tree");
    treeTable->addStyleClass("tree_left");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox
    auto root = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAll"));
    treeTable->setTreeRoot(std::move(root), "Raster");


    // création des groupes de couches avec checkbox qui permet de toutes les selectionner en un click
    // aptitude FEE
    auto grAptFEE = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAptFEE"));
    WTreeTableNode *grAptFEE_ = grAptFEE.get();
    std::unique_ptr<WCheckBox> checkAptFEE = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptFEE_ = checkAptFEE.get();
    checkAptFEE_->changed().connect([=]{SelectLayerGroup(checkAptFEE_->isChecked(),TypeLayer::FEE,"FEE");});
    grAptFEE->setColumnWidget(1, std::move(checkAptFEE));

    // aptitude CS

    auto grAptCS = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAptCS"));
    WTreeTableNode *grAptCS_ = grAptCS.get();
    std::unique_ptr<WCheckBox> checkAptCS = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkAptCS_ = checkAptCS.get();
    checkAptCS_->changed().connect([=]{SelectLayerGroup(checkAptCS_->isChecked(),TypeLayer::CS,"CS");});
    grAptCS->setColumnWidget(1, std::move(checkAptCS));

    // habitats, potentiel sylvicole
    auto grKK = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheKKCS"));
    WTreeTableNode *grKK_ = grKK.get();
    std::unique_ptr<WCheckBox> checkKK = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkKK_ = checkKK.get();
    checkKK_->changed().connect([=]{SelectLayerGroup(checkKK_->isChecked(),TypeLayer::KK,"");});
    grKK->setColumnWidget(1, std::move(checkKK));

    // diagnostic Stationnel
    auto grSt = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheThem"));
    WTreeTableNode *grSt_ = grSt.get();
    std::unique_ptr<WCheckBox> checkSt = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkSt_ = checkSt.get();
    checkSt_->changed().connect([=]{SelectLayerGroup(checkSt_->isChecked(),TypeLayer::Thematique,"");});
    grSt->setColumnWidget(1, std::move(checkSt));

    // description du peuplement
    auto grPeup = cpp14::make_unique<WTreeTableNode>(tr("groupeCouchePeup"));
    WTreeTableNode *grPeup_ = grPeup.get();
    std::unique_ptr<WCheckBox> checkPeup = cpp14::make_unique<WCheckBox>();
    WCheckBox * checkPeup_ = checkPeup.get();
    checkPeup_->changed().connect([=]{SelectLayerGroup(checkPeup_->isChecked(),TypeLayer::Peuplement,"");});
    grPeup->setColumnWidget(1, std::move(checkPeup));

    //std::cout << " loop on layers in select layer 4 download"<< std::endl;
    for (Layer * l : mVpLs){
        std::string aCode=l->getCode();
        switch (l->Type()){
        case TypeLayer::FEE:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grAptFEE_->addChildNode(std::move(node1));
            if (mLayersCBox.find(std::vector<std::string> {aCode,"FEE"})!=mLayersCBox.end()){
                WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,"FEE"});
                if (isSelected(l->getCode(),"FEE")){check1_->setChecked();}
                check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode,"FEE");});
                node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
            } else {
                std::cout << "pas bon " << std::endl;
            }
            break;
        }
        case TypeLayer::CS:{
            if (ModeExpert){
                auto node2 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
                auto node2_ = node2.get();
                grAptCS_->addChildNode(std::move(node2));
                WCheckBox * check2_ = mLayersCBox.at(std::vector<std::string> {aCode,"CS"});
                check2_->changed().connect([=]{SelectLayer(check2_->isChecked(),aCode,"CS");});
                node2_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check2_));
                break;
            }
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
            if (ModeExpert){
                auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
                auto node1_ = node1.get();
                grKK_->addChildNode(std::move(node1));
                WCheckBox * check1_ = mLayersCBox.at(std::vector<std::string> {aCode,""});
                if (isSelected(l->getCode())){check1_->setChecked();}
                check1_->changed().connect([=]{SelectLayer(check1_->isChecked(),aCode);});
                node1_->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(check1_));
                break;
            }
        }
        case TypeLayer::Peuplement:{
            auto node1 = cpp14::make_unique<WTreeTableNode>(l->getShortLabel());
            auto node1_ = node1.get();
            grPeup_->addChildNode(std::move(node1));
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
    if (ModeExpert) treeTable->treeRoot()->addChildNode(std::move(grAptCS));
    treeTable->treeRoot()->addChildNode(std::move(grSt));
    if (ModeExpert) treeTable->treeRoot()->addChildNode(std::move(grKK));
    treeTable->treeRoot()->addChildNode(std::move(grPeup));
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
    //std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;
    // pourrait envoyer un signal au widget upload pour transmettre le nombre de couches sélectionnées pour affichage
}

/*
void selectLayers::updateSL(std::vector<Layer *> aVpLs){
    mVpLs=aVpLs;
    mSelectedLayers.clear();
    for (auto kv :mLayersCBox){
        delete kv.second;
    }
    std::cout << "done update SL" << std::endl;
}
*/

void selectLayers::SelectLayerGroup(bool select,TypeLayer aType,std::string aMode){
    for (Layer * l : mVpLs){
        if (l->Type()==aType){
            SelectLayer(select,l->getCode(),aMode,false);
        }
    }
    //std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;
}


// pour envoyer la liste des raster à uploadcarte
std::vector<rasterFiles> selectLayers::getSelectedRaster(){
    std::vector<rasterFiles> aRes;
    for (Layer * l : mVpLs){
        aRes.push_back(l->getRasterfile());
    }
    return aRes;
}

void groupLayers::updateGL(){

    // clear
    for (auto p :  mVLs) { delete p; }
    mVLs.clear();
    mParent->clear();

    /* Liste cartes 1	*/
    std::unique_ptr<Wt::WTree> tree = Wt::cpp14::make_unique<Wt::WTree>();
    tree->setSelectionMode(Wt::SelectionMode::Extended);
    tree->addStyleClass("tree_left");
    //auto folderIcon = Wt::cpp14::make_unique<Wt::WIconPair>("icons/yellow-folder-closed.png", "icons/yellow-folder-open.png", false);
    //std::cout << "g1" << std::endl;
    auto main_node = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAll")); // std::move(folderIcon) // pour mettre des icones ouvert/fermé !
    tree->setTreeRoot(std::move(main_node));
    tree->treeRoot()->label()->setTextFormat(Wt::TextFormat::Plain);
    tree->treeRoot()->setLoadPolicy(Wt::ContentLoading::NextLevel);
    auto node1 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheThem"));
    auto node1_ = tree->treeRoot()->addChildNode(std::move(node1));
    node1_->addStyleClass("tree_node");
    tree->treeRoot()->expand();

    auto node0 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCouchePeup"));
    auto node0_ = tree->treeRoot()->addChildNode(std::move(node0));
    node0_->addStyleClass("tree_node");

    // carte IGN
    /* maintenant elle est dans la liste des cartes via la BD
    WText *label = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
    label->clicked().connect([this]{clickOnName("IGN",TypeLayer::Externe);});
    label->setTextAlignment(Wt::AlignmentFlag::Left);
    mVLs.push_back(new Layer(this,"IGN",label,TypeLayer::Externe));
    */

    // creation des layers pour les KK du CS
    if (ModeExpert){
        for (auto & pair : *mDico->codeKK2Nom()){
            WText *label;
            label = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
            mVLs.push_back(new Layer(this,pair.first,label,TypeLayer::KK));
            std::string aCode=pair.first;
            label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::KK);});
        }
    }

    // ajout des cartes "FEE" ; NT NH Topo AE SS

    for (auto & pair : *mDico->RasterType()){
        // si pas mode expert, on n'utilise pas les couches du catalogue de station
        if ((ModeExpert || pair.first.substr(0,2)!="CS") && mDico->rasterCat(pair.first)!="Peuplement"){
            WText *label;
            label = node1_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::Thematique);
            std::string aCode=pair.first;
            // un peu bidouille mais le typelayer de MNH est peuplement et il est redéfini dans le constructeur de layer
            TypeLayer type= aL->Type();
            label->clicked().connect([this,aCode,type]{clickOnName(aCode,type);});
            mVLs.push_back(aL);
        }
    }

    node1_->expand();

    for (auto & pair : *mDico->RasterType()){
        // couche catégorie peuplements
        if (mDico->rasterCat(pair.first)=="Peuplement"){
            WText *label;
            label = node0_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::Thematique);
            std::string aCode=pair.first;
            // un peu bidouille mais le typelayer de MNH et le masque forestier sont peuplement et il est redéfini dans le constructeur de layer
            TypeLayer type= aL->Type();
            label->clicked().connect([this,aCode,type]{clickOnName(aCode,type);});
            mVLs.push_back(aL);
        }
    }

    node0_->expand();

    auto node2 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptFEE"));
    auto node2_ = tree->treeRoot()->addChildNode(std::move(node2));
    node2_->addStyleClass("tree_node");

    auto node3 = Wt::cpp14::make_unique<Wt::WTreeNode>(tr("groupeCoucheAptCS"));
    auto node3_ = node3.get();
    node3_->addStyleClass("tree_node");
    if (ModeExpert){
        node3_ = tree->treeRoot()->addChildNode(std::move(node3));
    }

    // creation des layers pour les essences qui ont des aptitudes
    for (auto & pair : *mDico->code2Nom()){
        cEss ess(pair.first,mDico);
        //std::cout << "fee" << std::endl;
        if (ess.hasFEEApt()){
            WText *label;

            label = node2_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
            Layer  * aL= new Layer(this,pair.first,label,TypeLayer::FEE);
            mVLs.push_back(aL);
            std::string aCode=pair.first;
            label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::FEE);});
        }
        //std::cout << "cs" << std::endl;
        if (ModeExpert){
            if (ess.hasCSApt()){
                WText *label;
                label = node3_->addChildNode(Wt::cpp14::make_unique<Wt::WTreeNode>(""))->label();
                Layer  * aL= new Layer(this,pair.first,label,TypeLayer::CS);
                mVLs.push_back(aL);
                std::string aCode=pair.first;
                label->clicked().connect([this,aCode]{clickOnName(aCode,TypeLayer::CS);});
            }
        }

    }
    //std::cout << "done nodeitem" << std::endl;
    mParent->addWidget(std::move(tree));
    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep2")));
    mParent->addWidget(cpp14::make_unique<WText>(tr("coucheStep3")));
    WPushButton * bExportTiff = mParent->addWidget(cpp14::make_unique<WPushButton>("Télécharger"));
    //bExportTiff->disable();

    // pour l'instant tout passe par le slot JS qui renvoi un extent valide avant d'effectuer le crop et l'envoi de la carte à l'utilisateur
    // c'est pour éviter que l'extent ne soit pas "à jour" avant le crop
    bExportTiff->clicked().connect(this->slot);
    //bExportTiff->clicked().connect(this,&groupLayers::updateMapExtentAndCropIm);

    // création des arbres pour sélection des couches - ces objets sont affiché ailleur
    if (mSelect4Download) delete mSelect4Download;
    if (mSelect4Stat) delete mSelect4Stat;

    mSelect4Stat= new selectLayers4Stat(this);
    mSelect4Download= new selectLayers4Download(this);

}

Layer * groupLayers::getActiveLay(){
    Layer * aRes=NULL;
    for (Layer * l : mVLs){
        if (( l->IsActive())){aRes=l;break;}
    }
    return aRes;
}

void groupLayers::exportLayMapView(){

    std::cout << "exportLayMapView " << std::endl;

    Layer * l=getActiveLay();// attention, si on vient d'ouvrir le soft, aucune layer n'est actives!! gerer les ptr null
    if (l && l->Type()!=TypeLayer::Externe){
        // crop layer and download

        // crée l'archive
        std::string archiveFileName = mDico->File("TMPDIR")+"/"+l->getCode()+".zip";
        std::string aCroppedRFile = mDico->File("TMPDIR")+"/"+l->getCode()+"_crop.tif";
        std::string mClientName=l->getCode()+"_crop";
        rasterFiles r= l->getRasterfile();
        if ( cropIm(l->getPathTif(), aCroppedRFile, mMapExtent)){
            ZipArchive* zf = new ZipArchive(archiveFileName);
            zf->open(ZipArchive::WRITE);
            // pour bien faire; choisir un nom qui soit unique, pour éviter conflict si plusieurs utilisateurs croppent la mm carte en mm temps
            zf->addFile(mClientName+".tif",aCroppedRFile);
            if (r.hasSymbology()){zf->addFile(mClientName+".qml",r.symbology());}
            m_app->processEvents();
            zf->close();
            delete zf;
            WFileResource *fileResource = new Wt::WFileResource("plain/text",archiveFileName);
            fileResource->suggestFileName(mClientName+".zip");
            m_app->redirect(fileResource->url());
        }

    }else {
        Wt::WMessageBox * messageBox = this->addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                                          "Erreur",
                                                          "<p> Cette couche ne peut être téléchargé, veillez essayer avec une autres couche.</p>",
                                                          Wt::Icon::Critical,
                                                          Wt::StandardButton::Ok));

        messageBox->setModal(false);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    }
}


bool cropIm(std::string inputRaster, std::string aOut, OGREnvelope ext){
    bool aRes(0);
    std::cout << " cropIm" << std::endl;
    GDALAllRegister();
    if (exists(inputRaster)){
        double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
        const char *inputPath=inputRaster.c_str();
        const char *cropPath=aOut.c_str();
        GDALDataset *pInputRaster, *pCroppedRaster;
        GDALDriver *pDriver;
        const char *pszFormat = "GTiff";
        pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
        pInputRaster = (GDALDataset*) GDALOpen(inputPath, GA_ReadOnly);
        double transform[6], tr1[6];
        pInputRaster->GetGeoTransform(transform);
        pInputRaster->GetGeoTransform(tr1);
        //adjust top left coordinates
        transform[0] = ext.MinX;
        transform[3] = ext.MaxY;
        //determine dimensions of the new (cropped) raster in cells
        int xSize = round(width/transform[1]);
        int ySize = round(height/transform[1]);
        //std::cout << "xSize " << xSize << ", ySize " << ySize << std::endl;
        //create the new (cropped) dataset
        if (xSize>0 && ySize>0 && xSize <25000 && ySize<25000){
            pCroppedRaster = pDriver->Create(cropPath, xSize, ySize, 1, GDT_Byte, NULL); //or something similar

            pCroppedRaster->SetProjection( pInputRaster->GetProjectionRef() );
            pCroppedRaster->SetGeoTransform( transform );

            int xOffset=round((transform[0]-tr1[0])/tr1[1]);
            int yOffset=round((transform[3]-tr1[3])/tr1[5]);
            float *scanline;
            scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
            // boucle sur chaque ligne
            for ( int row = 0; row < ySize; row++ )
            {
                // lecture
                pInputRaster->GetRasterBand(1)->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                // écriture
                pCroppedRaster->GetRasterBand(1)->RasterIO( GF_Write, 0, row, xSize,1, scanline, xSize, 1,GDT_Float32, 0, 0 );
            }
            CPLFree(scanline);

            aRes=1;
        } else {
            std::cout << " crop du raster a échoué: taille pas correcte " << std::endl;
        }

        if( pCroppedRaster != NULL ){GDALClose( (GDALDatasetH) pCroppedRaster );}
        GDALClose(pInputRaster);
    } else {
        std::cout << " attention, un des fichiers input n'existe pas : " << inputRaster << std::endl;
    }
    return aRes;
}




