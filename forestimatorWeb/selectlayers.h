#ifndef SELECTLAYERS_H
#define SELECTLAYERS_H
#include "cdicoapt.h"
#include "Wt/WContainerWidget.h"
#include "Wt/WTreeTable.h"
#include "Wt/WTreeTableNode.h"
#include "Wt/WCheckBox.h"
#include "Wt/WMessageBox.h"
#include "Wt/WTree.h"

class selectLayers;
class rasterFiles;
class groupLayers;

using namespace  Wt;

class selectLayers : public WContainerWidget {
public:
    selectLayers(cDicoApt * aDico);
    std::vector<std::shared_ptr<layerBase>> getSelectedLayer();

    int numSelectedLayer(){
        int aRes(0);
        for (auto & kv: mSelectedLayers){
            if (kv.second==true){aRes++;}
        }
        return aRes;
    }

protected:
    std::map<std::shared_ptr<layerBase>,bool> mSelectedLayers;
    // une map de pointeur vers checkbox qui est liée aux couches selectionnée
    std::map<std::shared_ptr<layerBase>,Wt::WCheckBox*> mLayersCBox;
    // une map de pointeur vers node qui est liée aux couches selectionnées
    std::map<std::shared_ptr<layerBase>,Wt::WTreeTableNode*> mLayersNode;

    std::map<TypeLayer,Wt::WTreeTableNode*> mLayerGroupNode;

    void SelectLayer(bool select,std::shared_ptr<layerBase> l,bool afficheMsg=true);
    void SelectLayerGroup(bool select,TypeLayer aType);

    bool isSelected(std::shared_ptr<layerBase> l){
        bool aRes(0);
        if (mSelectedLayers.find(l)!=mSelectedLayers.end()){
            aRes=mSelectedLayers.at(l);
        }
        return aRes;
    }
    std::vector<std::shared_ptr<layerBase>> mVpLs;
    int nbMax;
    Wt::WTreeTable * treeTable;
};

#endif // SELECTLAYERS_H
