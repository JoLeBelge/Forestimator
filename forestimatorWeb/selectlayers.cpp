#include "selectlayers.h"

std::vector<std::shared_ptr<Layer>> baseSelectLayers::getSelectedLayer(){
    std::vector<std::shared_ptr<Layer>> aRes;
    for (auto kv : mSelectedLayers){
        if (kv.second){
            aRes.push_back(kv.first);
        }
    }
    return aRes;
}

selectLayers::selectLayers(groupLayers * aGL){

    mGL=aGL;
    mVpLs=aGL->Layers();
    nbMax=15;

    // ajout des noeuds racine des groupes de couches et création de la checkbox pour le groupe
    for (auto & kv : mLayerGroupNode){
        WTreeTableNode * n=kv.second;
        WCheckBox * checkB =new Wt::WCheckBox();
        checkB->changed().connect([=]{SelectLayerGroup(checkB->isChecked(),kv.first);});
        n->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(checkB));
        treeTable->treeRoot()->addChildNode(std::unique_ptr<Wt::WTreeTableNode>(n));
        // pour les noeuds qu'on veux cacher lorsqu'on est pas en mode expert
        if (kv.first == TypeLayer::KK | kv.first == TypeLayer::CS ){
            mGL->changeExpertMode().connect(n,&Wt::WTreeNode::setNodeVisible);
        }
    }

    // création des noeuds pour chaque couche et de sa checkbox et ajout dans le treetable dans le bon groupe de couche
    for (std::shared_ptr<Layer> l : mVpLs){
        if (l->getCatLayer()!=TypeLayer::Externe && l->l4Stat()){
            bool selected(0);
            //if ((l->getCatLayer()==TypeLayer::FEE) && (l->Code()=="HE"| l->Code()=="CS" | l->Code()=="CP" | l->Code()=="EP" | l->Code()=="DO" | l->Code()=="ME")){ selected=1;}
            if ((l->Code()=="HE_FEE")| (l->Code()=="CS_FEE") | (l->Code()=="CP_FEE") | (l->Code()=="EP_FEE") | (l->Code()=="DO_FEE") | (l->Code()=="ME_FEE")){ selected=1;}
            if ((l->Code()=="COMPOALL")| (l->Code()=="dendro_vha")){ selected=1;}

            mSelectedLayers.emplace(std::make_pair(l,selected));
            // checkBox
            Wt::WCheckBox * checkB =new Wt::WCheckBox();
            checkB->setChecked(selected);
            checkB->changed().connect([=]{SelectLayer(checkB->isChecked(),l);});
            mLayersCBox.emplace(std::make_pair(l,checkB));
            // tree node
            Wt::WTreeTableNode * n=new Wt::WTreeTableNode(l->getShortLabel());
            l->changeExpertMode().connect(n,&Wt::WTreeTableNode::setNodeVisible);
            mLayersNode.emplace(std::make_pair(l,n));
            n->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(checkB));
            // ajout au noeux racine opportun
            mLayerGroupNode.at(l->getCatLayer())->addChildNode(std::unique_ptr<Wt::WTreeTableNode>(n));
        }
    }
}

void baseSelectLayers::SelectLayer(bool select,std::shared_ptr<Layer> l,bool afficheMsg){

    if (globTest){std::cout << "couche " << l->getLegendLabel() << ", baseSelectLayers::SelectLayer, select= " << select << std::endl;}
    if ((!select) |(nbMax>(numSelectedLayer()))){
        if (mSelectedLayers.find(l)!=mSelectedLayers.end()){
            // permet d'empêcher la selection des layers expert en mode non-expert
            if (l->isVisible()){
                mSelectedLayers.at(l)=select;
                mLayersCBox.at(l)->setChecked(select);
            }
        } else {
            std::cout << "SelectLayer : je ne trouve pas la couche " << l->getLegendLabel() << std::endl;
        }
    } else {
        // faire apparaitre un message à l'utilisateur, maximum nbMax
        // changer le status de la checkbox, remettre à false
        if (select) mLayersCBox.at(l)->setChecked(false);
        if (afficheMsg){
            auto messageBox =
                    addChild(std::make_unique<Wt::WMessageBox>(
                                 "Sélection des couches",
                                 "<p>Vous avez atteint le maximun de " + std::to_string(nbMax)+ " couches</p>"
                                                                                                "<p>Veillez déselectionner une couche avant d'en sélectionner une nouvelle</p>",
                                 Wt::Icon::Information,
                                 Wt::StandardButton::Ok));

            messageBox->setModal(true);
            messageBox->buttonClicked().connect([=] {
                removeChild(messageBox);
            });
            messageBox->show();
        }

    }
    if (globTest){std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;}
}

void baseSelectLayers::SelectLayerGroup(bool select,TypeLayer aType){
    for (std::shared_ptr<Layer> l : mVpLs){
        if (l->getCatLayer()==aType && l->l4Stat()){
            SelectLayer(select,l,false);
        }
    }
}


// pour envoyer la liste des raster à uploadcarte
/*std::vector<std::shared_ptr<layerBase>> baseSelectLayers::getSelectedLayerBase(){
    std::vector<layerBase> aRes;
    for (std::shared_ptr<Layer> l : getSelectedLayer()){
        aRes.push_back(l->getRasterfile());
    }
    return getSelectedLayer();
}*/
baseSelectLayers::baseSelectLayers(){

    setOverflow(Wt::Overflow::Auto);
    treeTable = addWidget(std::make_unique<WTreeTable>());
    //treeTable->resize(300, 250);
    treeTable->setHeight(241);
    treeTable->setStyleClass("tree");
    treeTable->addStyleClass("tree_left");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox
    auto root = std::make_unique<WTreeTableNode>(tr("groupeCoucheAll"));
    treeTable->setTreeRoot(std::move(root), "Raster");
    treeTable->treeRoot()->expand();

    mLayerGroupNode.emplace(std::make_pair(TypeLayer::KK,new WTreeTableNode(tr("groupeCoucheKKCS"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::Station,new WTreeTableNode(tr("groupeCoucheThem"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::FEE,new WTreeTableNode(tr("groupeCoucheAptFEE"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::CS,new WTreeTableNode(tr("groupeCoucheAptCS"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::Peuplement,new WTreeTableNode(tr("groupeCouchePeup"))));

}
