#ifndef SELECTLAYERS_H
#define SELECTLAYERS_H
#include "grouplayers.h"

class selectLayers;
class selectLayers4Stat;
class selectLayers4Download;
class rasterFiles;

class selectLayers : public WContainerWidget{
public:
    selectLayers();
    std::vector<rasterFiles> getSelectedRaster();
    std::vector<Layer*> getSelectedLayer();

    int numSelectedLayer(){
        int aRes(0);
        for (auto & kv: mSelectedLayers){
            if (kv.second==true){aRes++;}
        }
        return aRes;
    }

protected: // les classes qui héritent en mode public peuvent avoir accès

    std::map<Layer*,bool> mSelectedLayers;
    // une map de pointeur vers checkbox qui est liée aux couches selectionnée
    std::map<Layer*,Wt::WCheckBox*> mLayersCBox;
    // une map de pointeur vers node qui est liée aux couches selectionnées
    std::map<Layer*,Wt::WTreeTableNode*> mLayersNode;

    std::map<TypeLayer,Wt::WTreeTableNode*> mLayerGroupNode;

    void SelectLayer(bool select,Layer* l,bool afficheMsg=true);
    void SelectLayerGroup(bool select,TypeLayer aType);

    bool isSelected(Layer* l){
        bool aRes(0);
        if (mSelectedLayers.find(l)!=mSelectedLayers.end()){
            aRes=mSelectedLayers.at(l);
        }
        return aRes;
    }
    std::vector<Layer*> mVpLs;
    int nbMax;

    Wt::WTreeTable * treeTable;
    groupLayers * mGL;

};

class selectLayers4Stat : public selectLayers{
public:
    selectLayers4Stat(groupLayers * aGL);
private:

};
class selectLayers4Download : public selectLayers{
public:
    selectLayers4Download(groupLayers * aGL);
private:

};

#endif // SELECTLAYERS_H
