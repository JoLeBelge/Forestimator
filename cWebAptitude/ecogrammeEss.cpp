#include "ecogrammeEss.h"

EcogrammeEss::EcogrammeEss(cEss * aEss, ST * aStation):mEss(aEss),mDico(aEss->Dico()),mST(aStation)
{ 
    resize(200,200*(15.0/7.0));
    //resize(this->parent()->width(),this->parent()->width()*15.0/7.0);
    //resize(pixPerLevel*7, pixPerLevel*15);   // Provide a default size. non sinon ne se resize pas.

    Width= this->width().toPixels();
    //std::cout << "ecogrammeEss width" << Width << std::endl;
    //int Height= paintDevice->height().toPixels();
    pixPerLevel=Width/7;
}

void EcogrammeEss::paintEvent(Wt::WPaintDevice *paintDevice){

    for (int i(0); i<this->areas().size();i++){
        this->removeArea(this->area(i));
    }
    //std::cout << "create painter" << std::endl;
    Wt::WPainter painter(paintDevice);

    draw(&painter);
    update();          // Trigger a repaint.
    //std::cout << "done \n" << std::endl;
}

void EcogrammeEss::draw(Wt::WPainter *painter){

    Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Black));
    pen0.setWidth(2);
    painter->setPen(pen0);

    //std::cout << "disque coloré pour niveaux" << std::endl;
    // dessin des disques coloré pour chaque NTxNH
    for (auto kvNH : *mDico->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
            for (auto kvNT : *mDico->NT()){
                int codeNT=kvNT.first;
                // on veut l'aptitude hydro-trophique, pas celle hierarchique Bioclim/HydroTroph.
                int apt=mEss->getApt(codeNT,codeNH,mST->mZBIO,false);
                std::shared_ptr<color> colApt=mDico->Apt2col(apt);
                int R,G,B;
                colApt->set(R,G,B);
                painter->setBrush(Wt::WBrush(Wt::WColor(R,G,B)));
                double x=(codeNT-7)*pixPerLevel+0.5;//+0.5*pixPerLevel+0.5;
                double y=mDico->posEcoNH(codeNH)*pixPerLevel+0.5;//+0.5*pixPerLevel+0.5;
                //std::cout << " elipse nt nh, " << codeNT << " , " << codeNH << ", apt " << apt << ", col " << colApt.cat() << "position " << x << " , " << y << std::endl;
                double w(0.9*pixPerLevel),h(0.9*pixPerLevel);
                painter->drawEllipse(x, y, w,h);

                std::unique_ptr<Wt::WRectArea> rectPtr = std::make_unique<Wt::WRectArea>(x, y, w,h);
                rectPtr->setToolTip(tr("eco.ntnh").arg(mDico->NT(codeNT)).arg(mDico->NH(codeNH)));
                rectPtr->setCursor(Wt::Cursor::IBeam);
                this->addArea(std::move(rectPtr));
            }
        }
    }
    //std::cout << "rectange mauve station" << std::endl;
    // dessin du rectangle autour de la station
    painter->setBrush(Wt::WBrush(Wt::StandardColor::Transparent));
    Wt::WPen pen = Wt::WPen();
    pen.setWidth(3);
    pen.setColor(Wt::WColor(Wt::StandardColor::DarkMagenta));
    painter->setPen(pen);

    if (mST->hasNH() && mST->hasNT()){
        int codeNT=mST->mNT;
        int codeNH=mST->mNH;
        double x=(codeNT-7)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        double y=mDico->posEcoNH(codeNH)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter->drawRect(x, y, pixPerLevel,pixPerLevel);
    }

    if (!mST->hasNH() && mST->hasNT()){
        int codeNT=mST->mNT;
        double x=(codeNT-7)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter->drawRect(x, 0, pixPerLevel,pixPerLevel*14);
    }

    if (mST->hasNH() && !mST->hasNT()){
        int codeNH=mST->mNH;
        double y=mDico->posEcoNH(codeNH)*pixPerLevel-0.5;//+0.5*pixPerLevel+0.5;
        //std::cout << "rect nt nh, " << codeNT << " , " << codeNH << ", position " << x << " , " << y << std::endl;
        painter->drawRect(0, y, pixPerLevel*6,pixPerLevel);
    }

}

// fonction pour dessiner l'écogramme dans un raster afin de l'exporter
void EcogrammeEss::draw(Wt::WPainter *painter, int x0, int y0, int x1, int y1, int yrha0){

    int apixPerLevelX=(x1-x0)/6;
    int apixPerLevelY=(y1-y0)/10; // les 10 niveaux classique, hors RHA
    Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Black));
    pen0.setWidth(2);
    painter->setPen(pen0);
    Wt::WPen penStation(Wt::WColor(Wt::StandardColor::DarkMagenta));
    penStation.setWidth(5);

    // dessin des disques coloré pour chaque NTxNH
    for (auto kvNH : *mDico->NH()){
        int codeNH=kvNH.first;
        if (codeNH!=0){
            for (auto kvNT : *mDico->NT()){
                int codeNT=kvNT.first;
                // on veut l'aptitude hydro-trophique, pas celle hierarchique Bioclim/HydroTroph.
                int apt=mEss->getApt(codeNT,codeNH,mST->mZBIO,false);
                std::shared_ptr<color> colApt=mDico->Apt2col(apt);
                //int R,G,B;
                //colApt->set(R,G,B);
                painter->setBrush(Wt::WBrush(Wt::WColor(colApt->mR,colApt->mG,colApt->mB)));
                double x=x0+(codeNT-7)*apixPerLevelX+0.5;
                // checkRHA
                double y(0);
                if (codeNH>16){
                    y=yrha0+(mDico->posEcoNH(codeNH)-11)*(double (apixPerLevelY)+0.5);// -11 car la position ecoNH dans le dictionnaire n'est pas appropriée ici
                } else {
                    y=y0+mDico->posEcoNH(codeNH)*(double (apixPerLevelY)+0.5);
                }

                Wt::WRectF ntnhRect(x,y,apixPerLevelX,apixPerLevelY);
                Wt::WRectF ntnhRectFull(x,y,apixPerLevelX,apixPerLevelY);
                reduire(&ntnhRect,5);
                //double w(0.9*apixPerLevelX),h(0.9*apixPerLevelY);
                //painter->drawEllipse(x, y, w,h);
                painter->drawEllipse(ntnhRect);
                // si c'est la station, ajout d'un deuxieme disque en mauve
                if (mST->hasNH() && mST->hasNT() && codeNH==mST->mNH && codeNT==mST->mNT){
                    painter->setPen(penStation);
                    painter->setBrush(Wt::WBrush(Wt::StandardColor::Transparent));
                    // méga foireux ce draw ellipse, pas intuitif du tout: x et y ne sont pas le centre de l'élipse mais la position ou passe le cercle.
                    // c'est mieux d'utiliser le draw ellipse avec un rectange
                    painter->drawEllipse(ntnhRectFull);
                    painter->setPen(pen0);
                }
            }
        }
    }

    // dessin du rectangle autour des stations si manque  nh ou nt
    painter->setBrush(Wt::WBrush(Wt::StandardColor::Transparent));
    painter->setPen(penStation);

    if (!mST->hasNH() && mST->hasNT()){
        int codeNT=mST->mNT;
        double x=x0+(codeNT-7)*apixPerLevelX-0.5;
        painter->drawRect(x, y0, apixPerLevelX,apixPerLevelY*10);
        painter->drawRect(x, yrha0, apixPerLevelX,apixPerLevelY*3);
    }

    if (mST->hasNH() && !mST->hasNT()){
        int codeNH=mST->mNH;
        double y=y0+mDico->posEcoNH(codeNH)*apixPerLevelY-0.5;
        painter->drawRect(x0, y, apixPerLevelX*6,apixPerLevelY);
    }
}

// réduction d'un rectangle de prop % de sa taille originelle
void reduire(Wt::WRectF * aRect,int prop){
    double bufX=((double)(aRect->width()*prop))/100.0;
    double bufY=((double)(aRect->height()*prop))/100.0;
    aRect->setHeight((aRect->height()*(100-prop)/100));
    aRect->setWidth((aRect->width()*(100-prop)/100));
    aRect->setX(aRect->x()+bufX);
    aRect->setY(aRect->y()+bufY);
}
