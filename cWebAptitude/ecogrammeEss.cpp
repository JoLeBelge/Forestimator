#include "ecogrammeEss.h"

EcogrammeEss::EcogrammeEss(cEss * aEss, ST * aStation):mEss(aEss),mDico(aEss->Dico()),mST(aStation)
{ 
    //setLayoutSizeAware(1);
    //std::cout << " \n\n\n mParent width" << mParent->width().toPixels() << std::endl;
    //resize(mParent->width().toPixels(),mParent->width().toPixels()*(15.0/7.0));
    resize(200,200*(15.0/7.0));
    //resize(this->parent()->width(),this->parent()->width()*15.0/7.0);
    //resize(pixPerLevel*7, pixPerLevel*15);   // Provide a default size. non sinon ne se resize pas.
}

void EcogrammeEss::paintEvent(Wt::WPaintDevice *paintDevice){

    for (int i(0); i<this->areas().size();i++){
      this->removeArea(this->area(i));
    }

    int Width= paintDevice->width().toPixels();
    std::cout << "ecogrammeEss width" << Width << std::endl;
    //int Height= paintDevice->height().toPixels();
    int pixPerLevel=Width/7;
    Wt::WPainter painter(paintDevice);
    painter.setPen(Wt::WColor(Wt::StandardColor::Black));

    // dessin des disques coloré pour chaque NTxNH
    for (auto kvNH : *mDico->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
        for (auto kvNT : *mDico->NT()){
            int codeNT=kvNT.first;
            int apt=mEss->getApt(codeNT,codeNH,mST->mZBIO);
            color colApt=mDico->Apt2col(apt);
            int R,G,B;
            colApt.set(R,G,B);
            painter.setBrush(Wt::WBrush(Wt::WColor(R,G,B)));
            double x=(codeNT-7)*pixPerLevel+0.5;//+0.5*pixPerLevel+0.5;
            double y=mDico->posEcoNH(codeNH)*pixPerLevel+0.5;//+0.5*pixPerLevel+0.5;
           // std::cout << " elipse nt nh, " << codeNT << " , " << codeNH << ", apt " << apt << ", col " << colApt.cat() << "position " << x << " , " << y << std::endl;
            double w(0.9*pixPerLevel),h(0.9*pixPerLevel);
            painter.drawEllipse(x, y, w,h);

            std::unique_ptr<Wt::WRectArea> rectPtr = Wt::cpp14::make_unique<Wt::WRectArea>(x, y, w,h);
            std::string aLabel("NT"+ mDico->NT(codeNT) +"   NH"+ mDico->NH(codeNH));
            rectPtr->setToolTip(aLabel);
            rectPtr->setCursor(Wt::Cursor::IBeam);
            this->addArea(std::move(rectPtr));
        }
        }
    }

    // dessin du rectangle autour de la station
    painter.setBrush(Wt::WBrush(Wt::StandardColor::Transparent));
    Wt::WPen pen = Wt::WPen();
    pen.setWidth(3);
    pen.setColor(Wt::WColor(Wt::StandardColor::DarkMagenta));
    painter.setPen(pen);

    if (mST->hasNH() && mST->hasNT()){
        int codeNT=mST->mNT;
        int codeNH=mST->mNH;
        double x=(codeNT-7)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        double y=mDico->posEcoNH(codeNH)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter.drawRect(x, y, pixPerLevel,pixPerLevel);
    }

    if (!mST->hasNH() && mST->hasNT()){
        int codeNT=mST->mNT;
        double x=(codeNT-7)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter.drawRect(x, 0, pixPerLevel,pixPerLevel*14);
    }

    if (mST->hasNH() && !mST->hasNT()){
        int codeNH=mST->mNH;
        double y=mDico->posEcoNH(codeNH)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter.drawRect(0, y, pixPerLevel*6,pixPerLevel);
    }

    update();          // Trigger a repaint.
    //std::cout << "done \n" << std::endl;
}
