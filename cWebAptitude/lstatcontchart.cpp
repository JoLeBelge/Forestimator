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
        for (int i(1) ; i+1 < seuilClasses.size() ; i++){
            aRange=roundDouble(seuilClasses.at(i),0)+"m-"+roundDouble(seuilClasses.at(i+1),0)+"m";
            aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(i)/((double)nbPix))));
        }
        aRange=">51m";
        aRes.push_back(std::make_pair(aRange,((double)clasOcc.at(clasOcc.size()-1)/((double)nbPix))));
    }

    return aRes;
}


void lStatContChart::predictHdom(){
    //std::cout << " predict Hdom depuis MNH2019" << std::endl;
    std::vector<double> aRes;

    // masque au format raster
    GDALDataset * mask = mLay->rasterizeGeom(mGeom);
    double transform2[6];
    mask->GetGeoTransform(transform2);
    double pixelWidth = transform2[1];
    double pixelHeight = -transform2[5];

    // découpe du masque en grille d'hexagone
    mVaddPol=hexGeombin(mask);

    // sauver sur disk pour vérification
    const char *pszFormat = "GTiff";
    GDALDriver * pDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if( pDriver != NULL )
    {
        std::string output(mLay->Dico()->File("TMPDIR")+"tmp.tif");
        const char *out=output.c_str();
        GDALDataset  * outM = pDriver->CreateCopy(out,mask,FALSE, NULL,NULL, NULL );
        if( outM != NULL ){ GDALClose( outM );}
    } else {
        std::cout << "pas chargé le driver tif" << std::endl;
    }

    GDALDataset  * DS = (GDALDataset *) GDALOpen( mLay->getPathTif().c_str(), GA_ReadOnly );
    if( DS == NULL )
    {
        std::cout << "je n'ai pas lu l'image " <<  mLay->getPathTif() << std::endl;
    } else {

        double transform[6];
        DS->GetGeoTransform(transform);

        int c(0);
        std::vector<int> toRm;
        for (OGRPolygon  * hex : mVaddPol){

            OGREnvelope ext;
            hex->getEnvelope(&ext);
            char** papszArgv = nullptr;
            papszArgv = CSLAddString(papszArgv, "-projwin"); //Selects a subwindow from the source image for copying but with the corners given in georeferenced coordinate
            papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinX).c_str());
            papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxY).c_str());
            papszArgv = CSLAddString(papszArgv, std::to_string(ext.MaxX).c_str());
            papszArgv = CSLAddString(papszArgv, std::to_string(ext.MinY).c_str());

            GDALTranslateOptions * option = GDALTranslateOptionsNew(papszArgv,nullptr);
            if (option){
                //std::cout <<" options parsées " << std::endl;
                GDALDatasetH DSmnhCrop = GDALTranslate("/vsimem/out",DS,option,nullptr);
                GDALDatasetH DSMaskCrop = GDALTranslate("/vsimem/out",mask,option,nullptr);
                if (DSmnhCrop && DSMaskCrop){
                    // j'ai toujours 2 raster a ouvrir conjointement
                    GDALDataset * MNH= GDALDataset::FromHandle(DSmnhCrop);
                    GDALDataset * MaskCrop= GDALDataset::FromHandle(DSMaskCrop);
                    float *scanline, *scanlineMask;
                    int xSize=MNH->GetRasterBand(1)->GetXSize();
                    int ySize=MNH->GetRasterBand(1)->GetYSize();
                    scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
                    scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
                    std::vector<double> aVHs;
                    // boucle sur chaque ligne
                    for ( int row = 0; row < ySize; row++ )
                    {
                        // lecture
                        MNH->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
                        // lecture du masque
                        MaskCrop->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
                        // boucle sur scanline et garder les pixels qui sont dans le polygone
                        for (int col = 0; col <  xSize; col++)
                        {
                            if (scanlineMask[col]==255){
                                OGRPoint pt(ext.MinX+col*pixelWidth,ext.MaxY-row*pixelWidth);
                                // check que le pixel est bien dans l'hexagone
                                if ( pt.Intersect(hex)){
                                    double aVal=scanline[ col ];
                                    aVHs.push_back(Dico()->H(aVal));
                                }
                            }
                        }
                    }
                    CPLFree(scanline);
                    CPLFree(scanlineMask);
                    GDALClose(MNH);
                    GDALClose(MaskCrop);

                    // prediction de Hdom si suffisament de valeur (si hexagone en bordure de polygone, pas l'hexa complêt)
                    double surf=aVHs.size()*pixelWidth*pixelHeight;
                    //std::cout << "surface nid abeille ; " << surf << std::endl;
                    // seuil de 800 m2
                    if (surf>800){
                        double hdom=predHdom(aVHs);
                        aRes.push_back(hdom);
                    } else {
                        // retirer l'hexagone de la liste
                        toRm.push_back(c);
                    }

                }
            }
            GDALTranslateOptionsFree(option);
            // hexagone suivant
            c++;
        }
        // trié en sens décroissant pour retirer les hexagones les plus loin dans le vecteur sans changer la positions des autres elem à retirer
        std::sort(toRm.begin(), toRm.end(), std::greater<>());
        for (int i:toRm){
            //std::cout << "enlever element " << i << " sur " << mVaddPol.size() << std::endl;
            OGRGeometryFactory::destroyGeometry(mVaddPol.at(i));
            mVaddPol.erase(mVaddPol.begin() + i);
        }
        GDALClose(DS);
        GDALClose(mask);
    }
    mStat=aRes;
}

bool lStatContChart::deserveChart(){
    bool aRes=mStat.size()>0;
    if (!aRes){ std::cout << "la couche " << mLay->getLegendLabel() << " ne mérite pas de graphique pour ses statistiques " << std::endl;}
    return aRes;
}

std::unique_ptr<WContainerWidget> lStatContChart::getResult(){
    std::cout << "lStatContChart::getResult() " << std::endl;
    std::unique_ptr<WContainerWidget> aRes= std::make_unique<Wt::WContainerWidget>();

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

    WContainerWidget * aContTable = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
    aContTable->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aContTable->setOverflow(Wt::Overflow::Auto);
    WTable * table =aContTable->addWidget(cpp14::make_unique<WTable>());

    table->elementAt(0, 0)->setColumnSpan(2);
    table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(0, 0)->setPadding(10);
    table->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.hdom.t")));

    basicStat bs(mStat);

    int c(1);
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Moyenne"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMean()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Coefficient de variation"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getCV()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Maximum"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMax()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Minimum"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getMin()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Nombre de Hdom mesuré (surface de 0.1 ha)"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getNb()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
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
        table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    }

    return std::move(aRes);
    //std::cout << "lStatContChart::getResult() done" << std::endl;
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


// cette fonctione ne fonctionne pas de manière identique sur le serveur qu'en local chez moi.
// chez moi ; 99 point. serveur ; 66 points, et manifestement pas au même endroit
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

    //std::cout << " MaxY " <<MaxY << " MinX " <<MinX << " MinY " <<MinY << " MaxX " << MaxX << std::endl;
    //int c(0);
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
               // c++;
                if (maskVal==255){

                    OGRPoint pt(x, y);
                    aRes.push_back(pt);
                    //std::cout << pt.getX() << "," << pt.getY() << std::endl;
                }
            }
        }
    }
    //std::cout << " c =" << c << ".." << std::endl;
    //std::cout << " hexbin generate " << aRes.size() << "points" << std::endl;
    return aRes;
}

std::vector<OGRPolygon *> hexGeombin(GDALDataset *mask){
    //std::cout << " hexGeombin sur le masque "<< std::endl;
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
    //std::cout << "hexGeombin generate " << aRes.size() << " hexagones" << std::endl;
    return aRes;

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

lStatCompoChart::lStatCompoChart(groupLayers * aGL, OGRGeometry * poGeom):mGeom(poGeom),mDico(aGL->Dico()){
    for (std::shared_ptr<Layer> l:aGL->getVpLs()){
        // on reconnais que c'est une carte de composition avec la taille du code et son préfix compo
        if(l->getCode().substr(0,5)=="COMPO" && l->getCode().size()==6){
            mVLay.push_back(l);
        }
    }
}


std::unique_ptr<WContainerWidget> lStatCompoChart::getResult(){
    std::cout << "lStatCompoChart::getResult() " << std::endl;
    std::unique_ptr<WContainerWidget> aRes= std::make_unique<Wt::WContainerWidget>();

    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV = aRes->setLayout(cpp14::make_unique<WVBoxLayout>());
    layoutV->addWidget(cpp14::make_unique<WText>("<h4>Composition</h4>"));

    WContainerWidget * aContTable = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
    aContTable->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aContTable->setOverflow(Wt::Overflow::Auto);
    WTable * table =aContTable->addWidget(cpp14::make_unique<WTable>());

    table->elementAt(0, 0)->setColumnSpan(2);
    table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(0, 0)->setPadding(10);
    table->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.compo.t")));

    int c(0);
    for (std::shared_ptr<Layer> l:mVLay){
        c++;
        // je vais utiliser une autre fonction qui lit la couche all_sp qui sert de masque, pour gerer les nd des différentes cartes.
        // basicStat stat= l->computeBasicStatOnPolyg(mGeom);
        basicStat stat=computeStatWithMasq(l,mGeom);
        table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>(l->getLegendLabel(false)));
        table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(stat.getMean()+"%"));
        table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    }
    return std::move(aRes);
}

// j'ai fait cette méthode car le calcul des stat sur les cartes de présence nécessite de charger la carte all_sp qui sert de masque. Sans ce masque je ne sais pas si j'ai 0 pour une ess car prob présence =0 ou si c'est un no data
basicStat lStatCompoChart::computeStatWithMasq(std::shared_ptr<Layer> aLay, OGRGeometry * poGeom){
    std::map<double,int> aMapValandFrequ;
    for (auto &kv : *aLay->mDicoVal){
        try {
            aMapValandFrequ.emplace(std::make_pair(std::stod(kv.second),0));
        }
        catch (const std::invalid_argument& ia) {
            std::cerr << "Invalid argument pour stod lStatCompoChart::computeStatWithMasq: " << ia.what() << '\n';
        }
    }
    // masque du polygone au format raster
    GDALDataset * maskPol = aLay->rasterizeGeom(mGeom);
    // retourner un DS pour la géométrie donnée
    GDALDataset * compo = getDSonEnv(aLay->getPathTif(),mGeom);
    GDALDataset * allEss = getDSonEnv(mDico->File("COMPOALL"),mGeom);

    /*
    std::cout << "maskPol " << maskPol->GetRasterXSize() << ", " <<  maskPol->GetRasterYSize() << std::endl;
    std::cout << "compo " << compo->GetRasterXSize() << ", " <<  compo->GetRasterYSize() << std::endl;
    std::cout << "allEss " << allEss->GetRasterXSize() << ", " << allEss->GetRasterYSize() << std::endl;
    */
    //determine dimensions of the tile
    int xSize = maskPol->GetRasterXSize();
    int ySize = maskPol->GetRasterYSize();

    float *scanline, *scanlineMask, *scanlineAllEss;
    scanline = (float *) CPLMalloc( sizeof( float ) * xSize );
    scanlineMask = (float *) CPLMalloc( sizeof( float ) * xSize );
    scanlineAllEss = (float *) CPLMalloc( sizeof( float ) * xSize );

    for ( int row = 0; row < ySize; row++ )
    {
        maskPol->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanlineMask, xSize,1, GDT_Float32, 0, 0 );
        compo->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1, scanline, xSize,1, GDT_Float32, 0, 0 );
        allEss->GetRasterBand(1)->RasterIO( GF_Read, 0, row, xSize, 1,scanlineAllEss, xSize,1, GDT_Float32, 0, 0 );
        // boucle sur scanline et garder les pixels qui sont dans le polygone
        for (int col = 0; col <  xSize; col++)
        {
            if (scanlineMask[col]==255){
                // test si la carte all_ess a une valeur pour cette position
                if (scanlineAllEss[col]>0){
                    double aVal=scanline[ col ];
                    //std::cout << " j'ai une valeur de prob de frequ de " << aVal << " et une val " << scanlineAllEss[col] << " pour la carte all_ess.tif " << std::endl;
                    if (aLay->mDicoVal->find(aVal)!=aLay->mDicoVal->end()){
                        try {
                            aMapValandFrequ.at(std::stod(aLay->mDicoVal->at(aVal)))++;
                        }
                        catch (const std::invalid_argument& ia) {
                            std::cerr << "Invalid argument pour stod lStatCompoChart::computeStatWithMasq, part2: " << ia.what() << '\n';
                        }
                    }
                }
            }
        }
    }
    CPLFree(scanline);
    CPLFree(scanlineMask);
    CPLFree(scanlineAllEss);

    if (maskPol!=NULL){GDALClose(maskPol);}
    if (compo!=NULL){GDALClose(compo);}
    if (allEss!=NULL){GDALClose(allEss);}

    if (aMapValandFrequ.size()>0) {return basicStat(aMapValandFrequ);} else {return  basicStat();}
}


