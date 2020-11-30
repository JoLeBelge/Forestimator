#include "lstatcontchart.h"
// hexagone ; apotheme et x qui sert pour les longeurs de d
//hexagone de 0.1ha de surface, soit 17 mètre d'apotheme car Aire=1/2*périmetre*apotheme et p=((a/sqrt(3))x2 x6coté, donc a = sqrt(sqrt(3)/6  * aire) et aire = 1000m2, voir https://fr.wikihow.com/calculer-l%27aire-d%27un-hexagone
double globa(17);
double globx=globa/std::sqrt(3);
// distance du centre à un sommet
double globd=2*globx;

lStatContChart::lStatContChart(std::shared_ptr<Layer> aLay, OGRGeometry * poGeom, TypeStat aType):mLay(aLay),mGeom(poGeom),mType(aType)
{
    std::cout << "lStatContChart::lStatContChart " << std::endl;
    switch (mType) {
    case TypeStat::HDOM:
        predictHdom(); // va calculer mStat et mVaddPol qui sont les hexagones à dessiner sur l'image statique
        mDistFrequ=computeDistrH();
        //computeDistrH();
        break;
    default:
        break;
    }

}


// le probleme des map c'est qu'elles sont ordonnées automatiquement avec leur clé, je veux pas.
std::vector<std::pair<std::string,double>> lStatContChart::computeDistrH(){

    std::vector<std::pair<std::string,double>> aRes;
    std::vector<double> seuilClasses{0,3,9,15,21,27,33,39,45,51};
    // vecteur plus long que le nombre de seuil de classe de 1
    std::vector<int> clasOcc{0,0,0,0,0,0,0,0,0,0,0};
    std::vector<double> aVHauteur;

    // c'est mon masque au format raster
    GDALDataset * mask = mLay->rasterizeGeom(mGeom);

    OGREnvelope ext;
    mGeom->getEnvelope(&ext);
    double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));

    GDALAllRegister();
    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( mLay->getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " << mLay->getPathTif() << std::endl;
    } else {
        GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

        double transform[6];
        mGDALDat->GetGeoTransform(transform);
        double xOrigin = transform[0];
        double yOrigin = transform[3];
        double pixelWidth = transform[1];
        double pixelHeight = -transform[5];

        //determine dimensions of the tile
        int xSize = round(width/pixelWidth);
        int ySize = round(height/pixelHeight);
        int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
        int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);

        float *scanline, *scanlineMask;
        scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
        scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
        // boucle sur chaque ligne
        for ( int row = 0; row < ySize; row++ )
        {
            // lecture
            mBand->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
            // lecture du masque
            mask->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
            // boucle sur scanline et garder les pixels qui sont dans le polygone
            for (int col = 0; col <  xSize; col++)
            {
                if (scanlineMask[col]==255){
                    double aVal=scanline[ col ];
                    aVHauteur.push_back(Dico()->H(aVal));
                }
            }
        }
        CPLFree(scanline);
        CPLFree(scanlineMask);

        mBand=NULL;
        GDALClose(mask);
        GDALClose(mGDALDat);

        // calcul de la distribution
        for (std::vector<double>::iterator it = aVHauteur.begin(); it < aVHauteur.end(); it++)
        {
            if (*it <= seuilClasses.at(0))
            {
               clasOcc.at(0)++;
            } else if (*it> seuilClasses.at(seuilClasses.size()-1)){
            clasOcc.at(clasOcc.size()-1)++;
            } else {
                 for (int i(0) ; i+1 < seuilClasses.size() ; i++){
                     if (*it > seuilClasses.at(i) && *it <= seuilClasses.at(i+1)){
                        clasOcc.at(i+1)++;
                     }
                 }
            }
        }

        /*for (int occ : clasOcc){
            std::cout << " occurence classe est " << occ << std::endl;
        }*/

        int nbPix=aVHauteur.size();
        // std::cout << " nombre de hauteur ;  " << nbPix<< std::endl;
        std::string aRange("<3m");
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(0)/((double)nbPix))));
        for (int i(0) ; i+1 < seuilClasses.size() ; i++){
        aRange=roundDouble(seuilClasses.at(i),0)+"m-"+roundDouble(seuilClasses.at(i+1),0)+"m";
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(i+1)/((double)nbPix))));
         }
        aRange=">51m";
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(clasOcc.size()-1)/((double)nbPix))));

        }

    return aRes;
}


void lStatContChart::predictHdom(){
    std::cout << " predict Hdom depuis MNH2019" << std::endl;
    std::vector<double> aRes;

    // masque au format raster
    GDALDataset * mask = mLay->rasterizeGeom(mGeom);
    double transform2[6];
    mask->GetGeoTransform(transform2);
    double xOriginMask = transform2[0];
    double yOriginMask = transform2[3];
    double pixelWidth = transform2[1];
    double pixelHeight = -transform2[5];

    // découpe du masque en grille d'hexagone
    mVaddPol=hexGeombin(mask);

    GDALDataset  * mGDALDat = (GDALDataset *) GDALOpen( mLay->getPathTif().c_str(), GA_ReadOnly );
    if( mGDALDat == NULL )
    {
        std::cout << "je n'ai pas lu l'image " <<  mLay->getPathTif() << std::endl;
    } else {
        GDALRasterBand * mBand = mGDALDat->GetRasterBand( 1 );

        double transform[6];
        mGDALDat->GetGeoTransform(transform);
        double xOrigin = transform[0];
        double yOrigin = transform[3];// max Y

        for (OGRPolygon  * hex : mVaddPol){
            //dimention du carré contenanl l'hexagone dans son entièretée
            OGREnvelope ext;
            hex->getEnvelope(&ext);
            double width((ext.MaxX-ext.MinX)), height((ext.MaxY-ext.MinY));
            int xSize = round(width/pixelWidth);
            int ySize = round(height/pixelHeight);
            // offset pour la carte
            int xOffset = int((ext.MinX - xOrigin) / pixelWidth);
            int yOffset = int((yOrigin - ext.MaxY ) / pixelHeight);
            // offset pour le masque
            int xOffsetMask = int((ext.MinX - xOriginMask) / pixelWidth);
            int yOffsetMask = int((yOriginMask - ext.MaxY ) / pixelHeight);

            float *scanline, *scanlineMask;
            scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
            scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
            std::vector<double> aVHs;
            // boucle sur chaque ligne
            for ( int row = 0; row < ySize; row++ )
            {
                // check effet de bord, pas correctement geré ; les centroide d'hexagone en bordure droite et gauche du masque ne peuvent être lue car la requete dépase la taille du masque
                // une solution serait de lire pixel par pixel au lieu de ligne par ligne, mais pour le moment je laisse ça à plus tard
                if (xOffsetMask+xSize <mask->GetRasterBand(1)->GetXSize() && row+yOffsetMask < mask->GetRasterBand(1)->GetYSize()){
                    //std::cout << " ligne sur MNH; xOffset " << xOffset << " yOffset " << row+yOffset << std::endl;
                    // lecture MNH
                    mBand->RasterIO( GF_Read, xOffset, row+yOffset, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                    // lecture du masque
                    mask->GetRasterBand(1)->RasterIO( GF_Read, xOffsetMask , row+yOffsetMask, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
                    // boucle sur scanline et garder les pixels qui sont ET dans le mask ET dans l'hexagone
                    // colorier ces pixels et sauver le masque pour contrôler que ça fonctionne?
                    for (int col = 0; col <  xSize; col++)
                    {
                        if (scanlineMask[col]==255){
                            OGRPoint pt(ext.MinX+col*pixelWidth,ext.MaxY-row*pixelWidth);
                            // check que le pixel est bien dans l'hexagone
                            if ( pt.Intersect(hex)){
                                double aVal=scanline[ col ];
                                // appliquer gain et offset et ajouter au vecteur
                                aVHs.push_back(Dico()->H(aVal));
                            }
                        }
                    }
                }
            }
            CPLFree(scanline);
            CPLFree(scanlineMask);
            // prediction de Hdom si suffisament de valeur (si hexagone en bordure de polygone, pas l'hexa complêt)
            double surf=aVHs.size()*pixelWidth*pixelHeight;
            //std::cout << "surface nid abeille ; " << surf << std::endl;
            // seuil de 800 m2
            if (surf>800){
                double hdom=predHdom(aVHs);
                aRes.push_back(hdom);
            }
        }
        GDALClose(mask);
        GDALClose(mGDALDat);
    }
    mStat=aRes;
}

std::unique_ptr<WContainerWidget> lStatContChart::getResult(){
    std::cout << "lStatContChart::getResult() " << std::endl;
    std::unique_ptr<WContainerWidget> aRes= std::make_unique<Wt::WContainerWidget>();
    /*
    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV = aRes->setLayout(cpp14::make_unique<WVBoxLayout>());
    layoutV->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));

    WContainerWidget * aCont = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = aCont->setLayout(cpp14::make_unique<WHBoxLayout>());
    // ajout de la carte pour cette couche

    staticMap sm(mLay,mGeom);
    // dessine les géométrie supplémentaire si nécessaire // pour le moment uniquement hexagone de Hdom
    //for (OGRPolygon pol : mVaddPol){
    if (mVaddPol.size()>0){
        sm.addPols(mVaddPol, Wt::StandardColor::DarkBlue);
    }
    //}

    Wt::WImage * im =layoutH->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLinkRel()),0);
    im->resize(350,350);

    //WContainerWidget * aContTable = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
    //aContTable->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    //aContTable->setOverflow(Wt::Overflow::Auto);
    WTable * table =layoutH->addWidget(cpp14::make_unique<WTable>());

    table->elementAt(0, 0)->setColumnSpan(2);
    table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(0, 0)->setPadding(10);
    table->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.hdom.t")));

    basicStat bs(mStat);

    int c(1);
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Moyenne"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMean()));
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Coefficient de variation"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getCV()));
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Maximum"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMax()));
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Minimum"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMin()));
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Nombre de Hdom mesuré (surface de 0.1 ha)"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getNb()));
    c++;
    // maintenant un tableau avec proportion par classe de hauteur;
    table->elementAt(c, 0)->setColumnSpan(2);
    table->elementAt(c, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(c, 0)->setPadding(10);
    table->elementAt(c,0)->addWidget(cpp14::make_unique<WText>("Répartition de la surface par classe de hauteur"));
    for (std::pair<std::string, double> p : mDistFrequ){
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>(p.first));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(roundDouble(100.0*p.second,0)+"%"));
    }
    */
    return std::move(aRes);
     std::cout << "lStatContChart::getResult() done" << std::endl;
}

// modèle de prédiction de hdom depuis MNH2019
double predHdom(std::vector<double> aVHs){
    //std::cout << "predit Hdom depuis un vecteur de hauteur photogrammétrique " ;
    double aRes(0.0);
    // tri du vecteur de hauteur puis sélection du 95ieme percentile
    std::sort(aVHs.begin(), aVHs.end());
    double pos95=((double) aVHs.size())*95.0/100.00;
    double h95= aVHs.at((int) pos95);
    //Hdom = 1,4 + 1,05 * CHM95  (erreur résiduelle : 1,35 m)
    double Hdom=1.4 + 1.05 * h95;
    aRes=Hdom;
    //std::cout << aRes << std::endl;
    return aRes;
}

std::vector<OGRPoint> hexbin(GDALDataset * mask){
    //std::cout << " hexbin sur le masque "<< std::endl;
    std::vector<OGRPoint> aRes;
    // nid d'abeille de 0.1 ha de surface.
    //double distEntreLignes=globd*sqrt(2);
    double distEntreLignes=globa;
    double distEntreColonnes=globd*2+globa;
    double transform[6];
    mask->GetGeoTransform(transform);
    double MaxY=transform[3];
    double MinX=transform[0];
    double MinY=transform[3]+transform[5]*mask->GetRasterBand(1)->GetYSize();
    double MaxX=transform[0]+transform[1]*mask->GetRasterBand(1)->GetXSize();

    double xOrigin = transform[0];
    double yOrigin = transform[3];
    double pixelWidth = transform[1];
    double pixelHeight = -transform[5];

    for (int i(0) ; i< ((MaxX-MinX)/(distEntreColonnes)); i++){
        for (int j(0) ; j< ((MaxY-MinY)/distEntreLignes); j++){
            double x=MinX+i*distEntreColonnes;
            // décallage horizontal une ligne sur 2. attention de ne pas sortir du raster masque alors.
            if ( j%2==0){x+=distEntreColonnes/2;}
            double y=MinY+j*distEntreLignes;

            // check que la valeur du mask est bien à 1 pour cette position
            int col = int((x - xOrigin) / pixelWidth);
            int row = int((yOrigin - y ) / pixelHeight);

            if (col<mask->GetRasterBand(1)->GetXSize() && row < mask->GetRasterBand(1)->GetYSize()){
                float *scanPix;
                scanPix = (float *) CPLMalloc( sizeof( float ) * 1 );
                // lecture du pixel
                mask->GetRasterBand(1)->RasterIO( GF_Read, col, row, 1, 1, scanPix, 1,1, GDT_Float32, 0, 0 );
                int maskVal=scanPix[0];
                if (maskVal==255){
                    OGRPoint pt(x, y);
                    aRes.push_back(pt);
                    //std::cout << pt.getX() << "," << pt.getY() << std::endl;
                }
            }
        }
    }
    return aRes;
}

std::vector<OGRPolygon *> hexGeombin(GDALDataset *mask){
    std::cout << " hexGeombin sur le masque "<< std::endl;

    std::vector<OGRPoint> VCentreHexagone=hexbin(mask);
    std::vector<OGRPolygon *> aRes(VCentreHexagone.size());
    int c(0);
    for (OGRPoint p : VCentreHexagone){
        //std::cout << " création d'un hexagone "<< std::endl;
        // création d'un hexagone
        OGRLinearRing * ring = new OGRLinearRing();
        OGRPolygon * hex= new OGRPolygon();
        // liste des 6 sommets ;
        // dimension de d= distance du centre de l'hex au sommet (pas la mm chose que apotheme) = 2x
        ring->addPoint(p.getX()+globx, p.getY()+globa);
        ring->addPoint(p.getX()+globd, p.getY());
        ring->addPoint(p.getX()+globx, p.getY()-globa);
        ring->addPoint(p.getX()-globx, p.getY()-globa);
        ring->addPoint(p.getX()-globd, p.getY());
        ring->addPoint(p.getX()-globx, p.getY()+globa);
        ring->closeRings();
        //std::cout << " ring done "<< std::endl;
        hex->addRingDirectly(ring);
       // delete ring;
        /* pour vérification dans Qgis
        char *toto;
        toto = hex.exportToJson();
        std::cout << toto << std::endl;
        */
        aRes[c]=hex;
        c++;
    }
    return aRes;
    std::cout << "done " << std::endl;
}

cDicoApt * lStatContChart::Dico(){return mLay->Dico();}

/*
OGRPoint * getCentroid(OGRPolygon * hex){
    //std::cout << "getCentroid " << std::endl;
    OGRPoint ptTemp1;
    std::vector<Wt::WLineF> aVLines;
    double x(1), y(1);
    int NumberOfVertices = hex->getExteriorRing()->getNumPoints();
    for ( int k = 0; k < NumberOfVertices; k++ )
    {
        hex->getExteriorRing()->getPoint(k,&ptTemp1);
        x+=ptTemp1.getX();
        y+=ptTemp1.getX();
    }
    x/=NumberOfVertices;
    y/=NumberOfVertices;
    return new OGRPoint(x,y);
}

// https://stackoverflow.com/questions/5193331/is-a-point-inside-regular-hexagon
bool IsInsideHexagon(float x0, float y0, float d, float x, float y) {
    float dx = std::abs(x - x0)/d;
    float dy = std::abs(y - y0)/d;
    float a = 0.25 * std::sqrt(3.0);
    return (dy <= a) && (a*dx + 0.25*dy <= 0.5*a);
}

bool InsideHexagonB(float x0, float y0, float x, float y,float d)
{
    float dx = std::abs(x - x0)/d;
    float dy = std::abs(y - y0)/d;
    // Check length (squared) against inner and outer radius
    float l2 = dx * dx + dy * dy;
    if (l2 > 1.0f) return false;
    if (l2 < 0.75f) return true; // (sqrt(3)/2)^2 = 3/4

    // Check against borders
    float px = dx * 1.15470053838f; // 2/sqrt(3)
    if (px > 1.0f || px < -1.0f) return false;

    float py = 0.5f * px + dy;
    if (py > 1.0f || py < -1.0f) return false;

    if (px - py > 1.0f || px - py < -1.0f) return false;

    return true;
}*/

