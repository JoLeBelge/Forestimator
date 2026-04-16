#include "layer.h"

Layer::Layer(std::shared_ptr<layerBase> aLB, WText * PWText):
   layerBase(aLB)
  ,mWtText(PWText)
  ,mIsVisible(1)
{
    mWtText->setText(mNomCourt);
    setActive(false);
}

void Layer::setActive(bool active){
    if (mWtText!=NULL){
        mWtText->setStyleClass(active ? "currentEss" : "ess");
        if (active) {mWtText->setToolTip(WString::tr("toolTipActiveLayer"));} else {mWtText->setToolTip(getLegendLabel(0));}
    }
}
