#ifndef ECOGRAMMEESS_H
#define ECOGRAMMEESS_H

#include <Wt/WImage.h>
#include <Wt/WAny.h>
#include <Wt/WSvgImage.h>

#include <Wt/WContainerWidget.h>
#include <Wt/WPaintDevice.h>
#include <Wt/WPaintedWidget.h>
#include <Wt/WPainter.h>
#include <Wt/WSpinBox.h>
#include <vector>
#include <Wt/WAbstractArea.h>
#include "cdicoapt.h"
#include <Wt/WRectArea.h>
#include <Wt/WRasterImage.h>

void reduire(Wt::WRectF * aRect,int prop);


class EcogrammeEss : public Wt::WPaintedWidget
{
public:
    EcogrammeEss(cEss * aEss, ST * aStation);


    // ça ne fonctionne que avec un layout, et le layout ce n'est pas ce que je veux pour cette partie
    virtual void layoutSizeChanged(int width, int height)
    {
        std::cout << "\n\n\n SIZE CHANGED " << width << " " << height << std::endl;
        WPaintedWidget::layoutSizeChanged(width, height);
    }

    void draw(Wt::WPainter *painter);

    // pour dessiner dans une image pour export dans pdf ; choix de la largeur de l'image
    //xy0 ; position en haut à gauche pour ecogramme principal. xy1 ; en bas à droite, pour eco principal
    void draw(Wt::WPainter *painter, int x0, int y0, int x1, int y1, int yrha0);

private:
    cEss * mEss;
    cdicoAptBase *mDico;
    //int mZbio,pixPerLevel;
    ST  * mST;
    int pixPerLevel,Width;
    //Wt::WContainerWidget * mParent;
    void paintEvent(Wt::WPaintDevice *paintDevice);
};

#endif // ECOGRAMMEESS_H
