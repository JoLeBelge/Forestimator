#include "layer.h"

Layer::Layer(std::shared_ptr<layerBase> aLB, WText * PWText):
   layerBase(aLB)
  ,mWtText(PWText)
  ,mIsVisible(1)
{
    //std::cout << "création de layer pour " << aCode << std::endl;
    //mLabel=mNom;
    mWtText->setText(mNomCourt);

    setActive(false);
    //std::cout << "done" << std::endl;
}
/*
Layer::Layer(std::string aCode):
   layerBase(aCode,aGroupL->Dico())
  ,mIsVisible(0)
{
    //mLabel=mNomCourt;
    mWtText=NULL;
    setActive(false);
}*/

Layer::Layer(groupLayers * aGroupL, std::shared_ptr<layerBase> aLB, WText * PWText):
    layerBase(aLB)
   ,mWtText(PWText)
   ,mIsVisible(1)
 {
     mWtText->setText(mNomCourt);
     setActive(false);
 }

void Layer::setActive(bool b){
    mActive=b;
    if (mWtText!=NULL){
        mWtText->setStyleClass(mActive ? "currentEss" : "ess");
        if (mActive) {mWtText->setToolTip(WString::tr("toolTipActiveLayer"));} else {mWtText->setToolTip(getLegendLabel(0));}
    }
}

void Layer::displayLayer() const{ 
    std::string JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
            "extent: extent,"+
            "title: 'MYTITLE',"+
            "source: new ol.source.TileWMS({"+
                 "preload: Infinity,"+
                "title: 'MYTITLE',"+
                "url: 'MYURL',"+
                "crossOrigin: 'null',"+
                "attributions: 'MYATTRIBUTION',"+
                "params: {"+
                  "'LAYERS': 'MYLAYER',"+
                  "'TILED': false,"+
                  //'TILED': false, // avant était à true mais ça faisait bugger cartoweb_topo
                  "'FORMAT': 'image/png'"+
                "},"+
                "tileGrid: tileGrid,"+
                "projection: 'EPSG:31370',"+
           " }),"+
            //opacity: Object.keys(activeLayers).length==0?1:0.5 // première image ; opaque. Les autres ; tranparence
           " opacity: 1"+
        "});";


    if (mTypeWMS==TypeWMS::ArcGisRest){
    JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
            "extent: extent,"+
            "title: 'MYTITLE',"+
              "  source: new ol.source.TileArcGISRest({"+
              "    attributions: 'MYATTRIBUTION',"+
               " url:'MYURL'"+
              "}),"+
             " opacity: 1"+
        "});";

    }

    JScommand+="activeLayers['MYCODE'] = activeLayer;updateGroupeLayers();";

   /* std::ifstream in(aFileIn);
    std::stringstream ss;
    ss << in.rdbuf();
    in.close();*/

    boost::replace_all(JScommand,"MYTITLE",this->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",mUrl);
    boost::replace_all(JScommand,"MYATTRIBUTION",mWMSattribution);
    boost::replace_all(JScommand,"MYCODE",mCode);

    mWtText->doJavaScript(JScommand);
}



