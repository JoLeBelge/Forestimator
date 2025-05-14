#include "layerstatchart.h"
extern bool globTest;
layerStatChart::layerStatChart(std::shared_ptr<Layer> aLay, std::map<std::string, int> aStat, OGRGeometry *poGeom):layerStat(aLay,aStat),rowAtMax(0),mGeom(poGeom)
{

    //std::cout << "création d'un layer StatChart pour " << mLay->getLegendLabel() << std::endl;
    mModel = std::make_shared<WStandardItemModel>();
    // pas sur que j'ai besoin de spécifier le proto
    //mModel->setItemPrototype(std::make_unique<WStandardItem>());

    // Configure the header.
    mModel->insertColumns(mModel->columnCount(), 2);
    //mModel->setHeaderData(0, WString(mLay->getLegendLabel()));
    mModel->setHeaderData(0, "Catégories");
    mModel->setHeaderData(1, WString("Proportions"));
    //mModel->setHeaderData(2, WString("Code couleur"));

    // Set data in the model.
    mModel->insertRows(mModel->rowCount(), mStatSimple.size());
    int row = 0;
    int aMax(1);

    //std::cout << "set data in model" << std::endl;
    for (auto & kv : mStatSimple){
        //clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        mModel->setData(  row, 0, WString(kv.first));
        mModel->setData(  row, 1, WString(kv.first), ItemDataRole::ToolTip);
        mModel->setData(  row, 1, kv.second);
        //mModel->setData(  row, 2, WString(""));
        // ça me retourne la couleur, moi j'aimerai le nom de cette couleur qui est utilisé pour l'ajout d'un style dans la styleClass de l'application
        std::shared_ptr<color> col=mLay->getColor(kv.first);
        mModel->itemFromIndex(mModel->index(row,1))->setStyleClass(col->getStyleNameShort());

        if (kv.second>aMax) {aMax=kv.second; rowAtMax=row;}
        row++;
    }

}

bool layerStatChart::deserveChart(){
    bool aRes=mStatSimple.size()>0;
    if (!aRes){ std::cout << "la couche " << mLay->Nom() << " ne mérite pas de graphique pour ses statistiques " << std::endl;}
    return aRes;
}

std::unique_ptr<WContainerWidget> layerStatChart::getChart(bool forRenderingInPdf){

    if (globTest) {std::cout << " creation d'un chart " << std::endl;}
    std::unique_ptr<WContainerWidget> aRes= std::make_unique<Wt::WContainerWidget>();
    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);
    staticMap sm(mLay,mGeom);
    WContainerWidget * aContTableAndPie ;

    if (forRenderingInPdf){// je pensais que le rendu pdf était en conflit avec les layouts, mais ça ne semble pas être ça...
        aRes->addWidget(std::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));
        WContainerWidget * aContIm =aRes->addWidget(std::make_unique<WContainerWidget>());
        Wt::WImage * im =aContIm->addWidget(std::make_unique<Wt::WImage>(sm.getWLinkRel()));
        im->resize(450,450);
        aContTableAndPie = aRes->addWidget(std::make_unique<WContainerWidget>());
    } else {
        WVBoxLayout * layoutV = aRes->setLayout(std::make_unique<WVBoxLayout>());
        layoutV->addWidget(std::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));
        WContainerWidget * aCont = layoutV->addWidget(std::make_unique<WContainerWidget>());
        WHBoxLayout * layoutH = aCont->setLayout(std::make_unique<WHBoxLayout>());
        // je dois ajouter un conteneur pour y mettre l'image dedans, sinon mise en page foireuse
        WContainerWidget * aContIm = layoutH->addWidget(std::make_unique<WContainerWidget>(),0);
        Wt::WImage * im =aContIm->addWidget(std::make_unique<Wt::WImage>(sm.getWLinkRel()));
        im->resize(450,450);
        aContTableAndPie = layoutH->addWidget(std::make_unique<WContainerWidget>());
    }
    aContTableAndPie->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aContTableAndPie->setOverflow(Wt::Overflow::Auto);
    if (mStatSimple.size()>0){
        if (mTypeVar==TypeVar::Classe){
            WTableView* table =aContTableAndPie->addWidget(std::make_unique<WTableView>());
            table->setMargin(10, Side::Top | Side::Bottom);
            table->setMargin(WLength::Auto, Side::Left | Side::Right);
            table->setAlternatingRowColors(0);
            table->setSortingEnabled(1,false);
            table->setSortingEnabled(0,false);// pas très utile
            table->setModel(mModel);
            // delegate ; met à 0 mes valeurs de pct dans la colonne, mais pour les labels de pct dans le graph ça fonctionne
            //std::shared_ptr<WItemDelegate> delegate = std::make_shared<WItemDelegate>();
            //delegate->setTextFormat("%.0f");
            //table->setItemDelegate(delegate);
            table->setColumnWidth(0, 200);
            table->setColumnWidth(1, 150);
            table->setRowHeight(28);
            table->setHeaderHeight(28);
            table->setWidth(200 + 150 + 14+2);
        } else if (mTypeVar==TypeVar::Continu){

            if(mLay->Code()=="MNH2019"){
                // pour MNH et MNT, pour l'instant  - recrée un vecteur stat, puis un model
                std::map<double, double> aStat;
                // je dois mettre et la hauteur en double, et le pct car sinon imprécision d'arrondi
                for (auto & kv : mStat){
                    try {
                        double h(std::stod(kv.first));

                        if (mLay->Code()=="MNH2019"){
                            if (h>3.0 && h<45){
                                aStat.emplace(std::make_pair(std::stod(kv.first),(100.0*kv.second)/mNbPix));

                            }
                        } else {aStat.emplace(std::make_pair(std::stod(kv.first),(100.0*kv.second)/mNbPix));}
                    }
                    catch (const std::invalid_argument& ia) {
                        std::cerr << "Invalid argument pour stod getChart: " << ia.what() << '\n';
                    }
                }

                std::shared_ptr<WStandardItemModel> model = std::make_shared<WStandardItemModel>();
                // Configure the header.
                model->insertColumns(model->columnCount(), 2);
                // Set data in the model.
                model->insertRows(model->rowCount(), aStat.size());
                int row = 0;
                for (auto & kv : aStat){
                    //clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
                    model->setData(  row, 0, kv.first);
                    model->setData(  row, 1, kv.second);
                    row++;
                }

                Chart::WCartesianChart *aChart = aContTableAndPie->addWidget(std::make_unique<Chart::WCartesianChart>());
                //aChart->setBackground(WColor(220, 220, 220));
                aChart->setModel(model);
                aChart->setXSeriesColumn(0);
                aChart->setLegendEnabled(true);
                aChart->setType(Chart::ChartType::Scatter);
                auto s = std::make_unique<Chart::WDataSeries>(1, Chart::SeriesType::Curve);
                s->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
                aChart->addSeries(std::move(s));
                aChart->resize(300, 300);    // WPaintedWidget must be given an explicit size.
                aChart->setMargin(20, Side::Top | Side::Bottom); // Add margin vertically.
                //aChart->setMargin(WLength::Auto, Side::Left | Side::Right); // Center horizontally. il faut mettre des marges, qui sont comtpée au départ du cammembert, pour mettre les label
                aChart->setMargin(50, Side::Left | Side::Right);

            }else if(mLay->Code()=="dendro_vha"){

            basicStat bs= mLay->computeBasicStatOnPolyg(mGeom);
            // ajout d'un tableau de synthèse très simple. moyenne, écart type, somme pour volume à l'hectare?
            WTable * table =aContTableAndPie->addWidget(std::make_unique<WTable>());

            table->elementAt(0, 0)->setColumnSpan(2);
            table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
            table->elementAt(0, 0)->setPadding(10);
            table->elementAt(0,0)->addWidget(std::make_unique<WText>(mLay->getLegendLabel(false)));
            table->elementAt(1, 0)->addWidget(std::make_unique<WText>("Moyenne [m3/ha]"));
            table->elementAt(1, 1)->addWidget(std::make_unique<WText>(bs.getMean()));
            table->elementAt(1, 1)->setPadding(10,Wt::Side::Left);
            table->elementAt(2, 0)->addWidget(std::make_unique<WText>("Ecart-type [m3/ha]"));
            table->elementAt(2, 1)->addWidget(std::make_unique<WText>(bs.getSd()));
            table->elementAt(2, 1)->setPadding(10,Wt::Side::Left);
            table->elementAt(3, 0)->addWidget(std::make_unique<WText>("Somme [m3]"));
            table->elementAt(3, 1)->addWidget(std::make_unique<WText>(bs.getSum()));
            table->elementAt(3, 1)->setPadding(10,Wt::Side::Left);
            table->elementAt(4, 0)->addWidget(std::make_unique<WText>("Surface sans donnée [m2]"));
            table->elementAt(4, 1)->addWidget(std::make_unique<WText>(bs.getNbNA()));
            table->elementAt(4, 1)->setPadding(10,Wt::Side::Left);
                }

            // fin variables continu
        }
        } else {
            aRes->addWidget(std::make_unique<WText>("Pas de statistique pour cette couche"));
        }
        return std::move(aRes);
    }

    std::unique_ptr<Wt::WContainerWidget> layerStatChart::getBarStat(){

        std::unique_ptr<Wt::WContainerWidget> aRes= std::make_unique<Wt::WContainerWidget>();
        aRes->addWidget(std::make_unique<batonnetApt>(this,mLay->Dico()->Dico_AptFull2AptAcro));

        return std::move(aRes);
    }


    staticMap::staticMap(std::shared_ptr<layerBase> aLay, OGRGeometry *poGeom, OGREnvelope *env, int aSz):mLay(aLay),mSx(aSz),mSy(aSz),ext(env){
        //std::cout << "staticMap::staticMap" << std::endl;
        std::string name0 = std::tmpnam(nullptr);
        std::string name1 = name0.substr(5,name0.size()-5);
        mFileName = mLay->Dico()->File("TMPDIR")+"/"+name1+".png";
        // nécessaire pour Wlink
        mFileNameRel = "tmp/"+name1+".png";

        if (ext==NULL){
            ext= new OGREnvelope;
            poGeom->getEnvelope(ext);
            // agrandir un peu l'extend de la carte car sinon le polygone peut-être partiellement visible seulemement
            int buf = (ext->MaxX-ext->MinX)/5;
            ext->MaxX+=buf;
            ext->MaxY+=buf;
            ext->MinX-=buf;
            ext->MinY-=buf;
        }
        // si la géométrie est un point, alors j'agrandi sinon division par 0== bug
        if (ext->MaxX-ext->MinX<10) {
            double bufPt(300);
            ext->MaxX+=bufPt;
            ext->MaxY+=bufPt;
            ext->MinX-=bufPt;
            ext->MinY-=bufPt;
        }


        // taille de l'emprise de l'image
        mWx=ext->MaxX-ext->MinX;
        mWy=ext->MaxY-ext->MinY;
        // on rectifie la taille de l'image en pixel en fonction de la forme de la zone
        mSy=(double)mSx*(mWy/mWx);

        //std::cout << ext->MinX << ", " << ext->MinY << ", " << ext->MaxX << ", " << ext->MaxY << std::endl;
        // d'abord transformer la couche wms en image locale, puis réouvrir et y dessiner le polygone
        if (mLay->wms2jpg(ext,mSx,mSy,mFileName)) {

            // création d'un wrasterImage et copier dedans l'image existante
            Wt::WRasterImage pngImage("png", mSx, mSy);
            WPainter painter(&pngImage);
            Wt::WPainter::Image imInit(mFileName,mFileName);
            Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,mSx, mSy);
            painter.drawImage(destinationRect,imInit);
            Wt::WPen pen0(Wt::WColor(Wt::StandardColor::Yellow));
            pen0.setWidth(3);
            painter.setPen(pen0);

            switch (poGeom->getGeometryType()){
            case (wkbPolygon):
            {
                OGRPolygon * pol =poGeom->toPolygon();
                drawPol(pol,&painter);
                break;
            }
            case wkbMultiPolygon:
            {
                OGRMultiPolygon * mulP = poGeom->toMultiPolygon();
                int n(mulP->getNumGeometries());
                for (int i(0);i<n;i++){
                    OGRGeometry * subGeom=mulP->getGeometryRef(i);
                    if (subGeom->getGeometryType()==wkbPolygon){
                        OGRPolygon * pol =subGeom->toPolygon();
                        drawPol(pol, &painter);
                    }
                }

                break;
            }
                // pour la carte générée pour analyse point, on ne dessine pas un polygone mais un cercle autour du point
            case wkbPoint:
            {
                OGRPoint * pt = poGeom->toPoint();
                // arg 3 et 4 pas intuitif, c'est la position ou doit se trouver le périmètre du cercle, pas du tout le radius ou autre
                //drawList.push_back(Magick::DrawableCircle(xGeo2Im(pt->getX()),yGeo2Im(pt->getY()), xGeo2Im(pt->getX())-10,yGeo2Im(pt->getY())));
                int x=xGeo2Im(pt->getX());
                int y=yGeo2Im(pt->getY());
                int buf(30);
                Wt::WRectF rect(x-buf,y-buf,2*buf,2*buf);
                painter.drawEllipse(rect);
                break;
            }
            case wkbLineString:
            {
                OGRPoint ptTemp1, ptTemp2;
                OGRLineString * l = poGeom->toLineString();
                std::vector<Wt::WLineF> aVLines;
                int NumberOfVertices =l->getNumPoints();
                for ( int k = 0; k < NumberOfVertices-1; k++ )
                {
                    l ->getPoint(k,&ptTemp1);
                    l->getPoint(k+1,&ptTemp2);
                    aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
                }
                painter.drawLines(aVLines);
                break;
            }

            default:
                std::cout << "Geometrie " << poGeom->getGeometryName() << " non pris en charge (static map)" << std::endl;

                break;
            }
            drawScaleLine(&painter);
            pngImage.done();
            std::ofstream f(mFileName, std::ios::out | std::ios::binary);
            pngImage.write(f);
            f.close();
        }
    }

    void staticMap::drawPol(OGRPolygon * pol, WPainter *painter){
        OGRPoint ptTemp1, ptTemp2;
        std::vector<Wt::WLineF> aVLines;
        int NumberOfVertices = pol->getExteriorRing()->getNumPoints();
        for ( int k = 0; k < NumberOfVertices-1; k++ )
        {
            pol->getExteriorRing()->getPoint(k,&ptTemp1);
            pol->getExteriorRing()->getPoint(k+1,&ptTemp2);
            aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
        }
        // ajout segment final
        pol->getExteriorRing()->getPoint(0,&ptTemp1);
        pol->getExteriorRing()->getPoint(NumberOfVertices-1,&ptTemp2);
        aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));

        for (int c(0);c<pol->getNumInteriorRings();c++){
            int NumberOfVertices =pol->getInteriorRing(c)->getNumPoints();;
            for ( int k = 0; k < NumberOfVertices-1; k++ )
            {
                pol->getInteriorRing(c)->getPoint(k,&ptTemp1);
                pol->getInteriorRing(c)->getPoint(k+1,&ptTemp2);
                aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
            }
            // ajout segment final
            pol->getInteriorRing(c)->getPoint(0,&ptTemp1);
            pol->getInteriorRing(c)->getPoint(NumberOfVertices-1,&ptTemp2);
            aVLines.push_back(WLineF(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
        }
        painter->drawLines(aVLines);
    }

    // ajouter toute la liste de polygone en 1 coup sinon trop long lecture - écriture de l'image
    void staticMap::addPols(std::vector<OGRPolygon *> vpol, Wt::WColor col){
        //std::cout << "staticMap::addPols " << std::endl;
        // création d'un wrasterImage et copier dedans l'image existante
        Wt::WRasterImage pngImage("png", mSx, mSy);
        WPainter painter(&pngImage);
        Wt::WPainter::Image imInit(mFileName,mFileName);
        Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,mSx, mSy);
        painter.drawImage(destinationRect,imInit);

        Wt::WPen pen0(col);
        pen0.setWidth(3);
        painter.setPen(pen0);

        for (OGRPolygon * pol : vpol){
            drawPol(pol,&painter);
        }

        pngImage.done();
        std::ofstream f(mFileName, std::ios::out | std::ios::binary);
        pngImage.write(f);
        f.close();
    }

    void staticMap::addImg(std::string afileName){
        std::cout << "staticMap::addImg" << std::endl;
        // création d'un wrasterImage et copier dedans l'image existante
        if (exists(afileName)){

            // image static map
            Wt::WRasterImage pngImage("png", mSx, mSy);
            WPainter painter(&pngImage);
            Wt::WPainter::Image imInit(mFileName,mFileName);
            Wt::WRectF destinationRect = Wt::WRectF(0.0,0.0,mSx, mSy);
            painter.drawImage(destinationRect,imInit);

            // ajout d'un logo
            Wt::WPainter::Image imLogo(afileName,afileName);
            Wt::WRectF destinationRectLogo = Wt::WRectF(mSx*9.0/10.0,mSy*9.0/10.0,mSx/10, mSy/10);
            painter.drawImage(destinationRectLogo,imLogo);

            pngImage.done();
            std::ofstream f(mFileName, std::ios::out | std::ios::binary);
            pngImage.write(f);
            f.close();
        } else {
            std::cout << "staticMap::addImg, je ne trouve pas l'image " << afileName << std::endl;
        }

    }

    void staticMap::drawScaleLine(WPainter *painter){

        std::vector<Wt::WLineF> aVLines;
        Wt::WPen pen0(WColor("black"));
        pen0.setWidth(4);
        painter->setPen(pen0);

        // determiner la taille appropriée en m de la scaleline
        // on veux que la scaleline fasse au max 0.75 de l'image
        int mul(10);
        int maxLength= (double) mWx *0.75;
        while ((maxLength / mul)>10){
            mul=mul*10;
        }
        int l= ((int) maxLength/mul)*mul;

        std::string label(std::to_string(l)+" m");

        double x0= ((double)mSx)*0.1;
        double y0= ((double)mSy)*0.9;
        // je sais pas comment choisir ma taille de texte, grr
        painter->drawText(x0, y0, mWx*0.75, 40,Wt::AlignmentFlag::Left,Wt::TextFlag::SingleLine,label);

        double y02= y0+40;
        int length= ((double)l*mSx)/mWx;

        aVLines.push_back(WLineF(x0,y02,x0+length,y02));
        // les délimitations de la scalebar
        int a(10);
        aVLines.push_back(WLineF(x0,y02-a,x0,y02+a));
        aVLines.push_back(WLineF(x0+length,y02-a,x0+length,y02+a));
        painter->drawLines(aVLines);
    }

    double staticMap::xGeo2Im(double x){
        return mSx*(x-ext->MinX)/mWx;
    }
    double staticMap::yGeo2Im(double y){
        return mSy-(mSy*(y-ext->MinY)/mWy);
    }

    std::string dToStr(double d){
        std::ostringstream streamObj;
        // Set Fixed -Point Notation
        streamObj << std::fixed;
        // Set precision to 1 digits
        streamObj << std::setprecision(1);
        //Add double to stream
        streamObj << d;
        return streamObj.str();
    }

    std::string  nth_letter(int n)
    {
        std::string aRes("A");
        if (n >= 1 && n <= 26) { aRes="abcdefghijklmnopqrstuvwxyz"[n-1];}
        return aRes;
    }


    void batonnetApt::paintEvent(Wt::WPaintDevice *paintDevice){
        Wt::WPainter painter(paintDevice);
        int xcumul(0);
        for (auto & kv : mLayStat->StatSimple()){

            //std::cout << "batonnet Aptitude " << kv.first << " " << kv.second << std::endl;

            std::string aCodeStr(kv.first);
            std::shared_ptr<color> col(mLayStat->Lay()->getColor(aCodeStr));
            painter.setBrush(Wt::WBrush(Wt::WColor(col->mR,col->mG,col->mB)));
            double width=kv.second*(mW/100.0);
            painter.drawRect(xcumul, 0, width, mH);

            std::unique_ptr<Wt::WRectArea> rectPtr = std::make_unique<Wt::WRectArea>(xcumul, 0, int(width), mH);
            std::string aLabel(kv.first+": "+ std::to_string(kv.second)+"%");
            rectPtr->setToolTip(aLabel);
            rectPtr->setCursor(Wt::Cursor::IBeam);
            this->addArea(std::move(rectPtr));

            // dessiner le code d'aptitude si il y a la place pour (seuil à 5 %)
            if (kv.second>5){
                std::string codeApt="";
                if (aptFull2aptAcro.find(kv.first)!=aptFull2aptAcro.end()){codeApt=aptFull2aptAcro.at(kv.first);}
                // je dois choisir entre Wt::AlignmentFlag::Middle et Wt::AlignmentFlag::Center, alors je prend middle et je change les coordonnées du box pour centrer
                painter.drawText(xcumul+width/2, 0, width, mH,Wt::AlignmentFlag::Middle,Wt::TextFlag::SingleLine,codeApt);
            }
            //std::cout <<  aCodeStr << ", batonnet de longeur " << width  << ", % de " << kv.second << std::endl;
            xcumul=xcumul+width;
        }
    }


