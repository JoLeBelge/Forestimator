#include "layer.h"

Layer::Layer(std::shared_ptr<layerBase> aLB, WText * PWText):
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
