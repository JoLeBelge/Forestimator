#include "selectlayers.h"


selectLayers4Stat::selectLayers4Stat(groupLayers * aGL){
    //std::cout << "creation de selectLayers4Stat " << std::endl;

    mGL=aGL;
    mVpLs=aGL->getVpLs();
    nbMax=10;

    // ajout des noeuds racine des groupes de couches et création de la checkbox pour le groupe
    for (auto & kv : mLayerGroupNode){
        WTreeTableNode * n=kv.second;
        WCheckBox * checkB =new Wt::WCheckBox();
        checkB->changed().connect([=]{SelectLayerGroup(checkB->isChecked(),kv.first);});
        n->setColumnWidget(1, std::unique_ptr<Wt::WCheckBox>(checkB));

        if (kv.first == TypeLayer::FEE | kv.first == TypeLayer::Peuplement ){
            treeTable->treeRoot()->addChildNode(std::unique_ptr<Wt::WTreeTableNode>(n));
        }
    }

    // création des noeuds pour chaque couche et de sa checkbox et ajout dans le treetable dans le bon groupe de couche
    for (Layer * l : mVpLs){

        if ((l->Type()!=TypeLayer::Externe) && (l->Type()==TypeLayer::FEE | l->Type()==TypeLayer::Peuplement)){
            bool selected(0);
            if (l->getCode()=="HE"| l->getCode()=="CS" | l->getCode()=="CP" | l->getCode()=="EP" | l->getCode()=="DO" | l->getCode()=="ME"){ selected=1;}

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
            mLayerGroupNode.at(l->Type())->addChildNode(std::unique_ptr<Wt::WTreeTableNode>(n));
        }
    }
}

std::vector<Layer *> selectLayers::getSelectedLayer(){
    std::vector<Layer*> aRes;
    for (auto kv : mSelectedLayers){
        if (kv.second){
            aRes.push_back(kv.first);
        }
    }
    return aRes;
}

selectLayers4Download::selectLayers4Download(groupLayers * aGL){

    mGL=aGL;
    mVpLs=aGL->getVpLs();
    nbMax=25;

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
    for (Layer * l : mVpLs){
        if (l->Type()!=TypeLayer::Externe){
            bool selected(0);
            if ((l->Type()==TypeLayer::FEE) && (l->getCode()=="HE"| l->getCode()=="CS" | l->getCode()=="CP" | l->getCode()=="EP" | l->getCode()=="DO" | l->getCode()=="ME")){ selected=1;}
            if (l->Type()==TypeLayer::Peuplement) { selected=1;}

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
            mLayerGroupNode.at(l->Type())->addChildNode(std::unique_ptr<Wt::WTreeTableNode>(n));
        }
    }
}

void selectLayers::SelectLayer(bool select,Layer* l,bool afficheMsg){

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
                    addChild(Wt::cpp14::make_unique<Wt::WMessageBox>(
                                 "Sélection des couches",
                                 "<p>Vous avez atteint le maximun de " + std::to_string(nbMax)+ " couches</p>"
                                                                                                "<p>Veillez déselectionner une couche avant d'en sélectionner une nouvelle</p>",
                                 Wt::Icon::Information,
                                 Wt::StandardButton::Ok));

            messageBox->setModal(false);
            messageBox->buttonClicked().connect([=] {
                removeChild(messageBox);
            });
            messageBox->show();
        }

    }
    //std::cout << "nombre de couches sélectionnées " << numSelectedLayer() << std::endl;
}

void selectLayers::SelectLayerGroup(bool select,TypeLayer aType){
    for (Layer * l : mVpLs){
        if (l->Type()==aType){
            SelectLayer(select,l,false);
        }
    }
}


// pour envoyer la liste des raster à uploadcarte
std::vector<rasterFiles> selectLayers::getSelectedRaster(){
    std::vector<rasterFiles> aRes;
    for (Layer * l : mVpLs){
        aRes.push_back(l->getRasterfile());
    }
    return aRes;
}

selectLayers::selectLayers(){

    setOverflow(Wt::Overflow::Auto);
    treeTable = addWidget(cpp14::make_unique<WTreeTable>());
    //treeTable->resize(300, 250);
    treeTable->setHeight(241);
    treeTable->setStyleClass("tree");
    treeTable->addStyleClass("tree_left");
    treeTable->tree()->setSelectionMode(SelectionMode::Extended);
    treeTable->addColumn("", 20); // colonne pour les checkbox
    auto root = cpp14::make_unique<WTreeTableNode>(tr("groupeCoucheAll"));
    treeTable->setTreeRoot(std::move(root), "Raster");
    treeTable->treeRoot()->expand();

    mLayerGroupNode.emplace(std::make_pair(TypeLayer::KK,new WTreeTableNode(tr("groupeCoucheKKCS"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::Thematique,new WTreeTableNode(tr("groupeCoucheThem"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::FEE,new WTreeTableNode(tr("groupeCoucheAptFEE"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::CS,new WTreeTableNode(tr("groupeCoucheAptCS"))));
    mLayerGroupNode.emplace(std::make_pair(TypeLayer::Peuplement,new WTreeTableNode(tr("groupeCouchePeup"))));

}