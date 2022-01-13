#include "statHdomCompo.h"

extern bool globTest;

statHdom::statHdom(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom, bool computeStat):statHdomBase(aLay,poGeom,computeStat)
{
    //std::cout << "statHdom::statHdom" << std::endl;
    //predictHdomHex(); // va calculer mStat et mVaddPol qui sont les hexagones à dessiner sur l'image statique

    if (computeStat){// pour gestion du polymorphysme avec classe statDendro
    prepareResult();
    }
}

statDendro::statDendro(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom):statDendroBase(aLay,poGeom,0)
{
    prepareResult();
}







void statHdom::prepareResult(){
    std::cout << "statHdom::prepareResult() " << std::endl;
    //std::unique_ptr<WContainerWidget>
    mResult= std::make_unique<Wt::WContainerWidget>();

    mResult->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    //aRes->setInline(0);
    mResult->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV = mResult->setLayout(cpp14::make_unique<WVBoxLayout>());
    layoutV->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));

    WContainerWidget * aCont = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = aCont->setLayout(cpp14::make_unique<WHBoxLayout>());
    // ajout de la carte pour cette couche

    staticMap sm(mLay,mGeom);
    // dessine les géométrie supplémentaire si nécessaire // pour le moment uniquement hexagone de Hdom

    if (mVaddPol.size()>0 & mVaddPol.size()<50){
        sm.addPols(mVaddPol, Wt::StandardColor::DarkBlue);
    }

    WContainerWidget * aContIm = layoutH->addWidget(cpp14::make_unique<WContainerWidget>(),0);
    Wt::WImage * im =aContIm->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLinkRel()));
    im->resize(450,450);

    WContainerWidget * aContTable = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
    aContTable->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aContTable->setOverflow(Wt::Overflow::Auto);
    WTable * table =aContTable->addWidget(cpp14::make_unique<WTable>());

    table->elementAt(0, 0)->setColumnSpan(2);
    table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(0, 0)->setPadding(10);
    table->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.hdom.t")));

    basicStat bs=bshdom();

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
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Nombre de Hdom mesuré (surface de 0.1 ha)"));// hexagones OLD OLD
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bs.getNb()));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    /*// autres variable dendro

    table->elementAt(c, 0)->setColumnSpan(2);
    table->elementAt(c, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(c, 0)->setPadding(10);
    table->elementAt(c,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.dendro.t")));
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("VHA moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bsDendro("vha").getMean()+ " m3/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("NHA moyen"));

    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bsDendro("nha").getMean()+ " tige/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("GHA moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bsDendro("gha").getMean()+ " m2/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Cmoy moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(bsDendro("cmoy").getMean()+ " cm"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;

    */

    // maintenant un tableau avec proportion par classe de hauteur;
    table->elementAt(c, 0)->setColumnSpan(2);
    table->elementAt(c, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(c, 0)->setPadding(10);

    table->elementAt(c,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.classeHauteur.t")));
    for (std::pair<std::string, double> p : mDistFrequ){
        c++;
        table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>(p.first));
        table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(roundDouble(100.0*p.second,0)+"%"));
        table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    }

    //return std::move(aRes);
    //std::cout << "lStatContChart::getResult() done" << std::endl;
}

basicStat statHdomBase::bshdom(){
    return bsDendro("hdom");
}
basicStat statHdomBase::bsDendro(std::string aVar){
    std::vector<double> aVVar;

    if (aVar=="hdom"){
        for (auto & ptCel : mStat){
            aVVar.push_back(ptCel->mHdom);
        }
    } else if (aVar=="vha"){
        for (auto & ptCel : mStat){
            aVVar.push_back(ptCel->mVHA);
        }
    } else if (aVar=="nha"){
        for (auto & ptCel : mStat){
            aVVar.push_back(ptCel->mNha);
        }
    } else if (aVar=="cmoy"){
        for (auto & ptCel : mStat){
            aVVar.push_back(ptCel->mCmoy);
        }
    } else if (aVar=="gha"){
        for (auto & ptCel : mStat){
            aVVar.push_back(ptCel->mGha);
        }
    }

    return basicStat(aVVar);
}





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
        y+=ptTemp1.getY();
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

statCompo::statCompo(cDicoApt * aDico, OGRGeometry * poGeom):mGeom(poGeom),mDico(aDico){
    for (auto kv:mDico->VlayerBase()){
        std::shared_ptr<layerBase> l=kv.second;
        // on reconnais que c'est une carte de composition avec la taille du code et son préfix compo
        if(l->Code().substr(0,5)=="COMPO" && l->Code().size()==6){
            mVLay.push_back(l);
        }
    }
}

statCompo::statCompo(std::vector<std::shared_ptr<layerBase>> VlayCompo, cDicoApt *aDico, OGRGeometry * poGeom):mGeom(poGeom),mDico(aDico){
    for (std::shared_ptr<layerBase> l:VlayCompo){
        mVLay.push_back(l);
    }
}


std::unique_ptr<WContainerWidget> statCompo::getResult(){
    std::cout << "statCompo::getResult() " << std::endl;
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
    for (std::shared_ptr<layerBase> l:mVLay){
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

std::string statCompo::getAPIresult(){
    std::string aRes("ess;prob\n");
    for (std::shared_ptr<layerBase> l:mVLay){
        // je vais utiliser une autre fonction qui lit la couche all_sp qui sert de masque, pour gerer les nd des différentes cartes.
        //std::cout << " calcul stat compo pour " << l->getShortLabel() << std::endl;
        basicStat stat=computeStatWithMasq(l,mGeom);
        aRes+=l->getShortLabel()+";"+stat.getMean()+"%\n";
    }
    return aRes;
}

// j'ai fait cette méthode car le calcul des stat sur les cartes de présence nécessite de charger la carte all_sp qui sert de masque. Sans ce masque je ne sais pas si j'ai 0 pour une ess car prob présence =0 ou si c'est un no data
basicStat statCompo::computeStatWithMasq(std::shared_ptr<layerBase> aLay, OGRGeometry * poGeom){
    std::map<double,int> aMapValandFrequ;
    for (auto &kv : aLay->getDicoVal()){
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
                    if (aLay->getDicoVal().find(aVal)!=aLay->getDicoVal().end()){
                        try {
                            aMapValandFrequ.at(std::stod(aLay->getDicoVal().at(aVal)))++;
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

void statDendro::prepareResult(){
    std::cout << "statHdom::prepareResult() " << std::endl;
    mResult= std::make_unique<Wt::WContainerWidget>();

    mResult->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    //aRes->setInline(0);
    mResult->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV = mResult->setLayout(cpp14::make_unique<WVBoxLayout>());
    //layoutV->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));

    WContainerWidget * aCont = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = aCont->setLayout(cpp14::make_unique<WHBoxLayout>());
    // ajout de la carte pour cette couche

    /*staticMap sm(mLay,mGeom);

    WContainerWidget * aContIm = layoutH->addWidget(cpp14::make_unique<WContainerWidget>(),0);
    Wt::WImage * im =aContIm->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLinkRel()));
    im->resize(450,450);
    */
    WContainerWidget * aContTable = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
    aContTable->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aContTable->setOverflow(Wt::Overflow::Auto);
    WTable * table =aContTable->addWidget(cpp14::make_unique<WTable>());

    table->elementAt(0, 0)->setColumnSpan(2);
    table->elementAt(0, 0)->setContentAlignment(AlignmentFlag::Top | AlignmentFlag::Center);
    table->elementAt(0, 0)->setPadding(10);
    table->elementAt(0,0)->addWidget(cpp14::make_unique<WText>(Wt::WString::tr("report.analyse.surf.dendro.t")));

    int c(1);
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Hdom moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(getHdom()+ " m"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("VHA moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(getVha()+ " m3/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("NHA moyen"));

    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(getNha()+ " tige/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("GHA moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(getGha()+ " m2/ha"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;
    table->elementAt(c, 0)->addWidget(cpp14::make_unique<WText>("Cmoy moyen"));
    table->elementAt(c, 1)->addWidget(cpp14::make_unique<WText>(getCmoy()+ " cm"));
    table->elementAt(c, 1)->setPadding(10,Wt::Side::Left);
    c++;

    //return std::move(aRes);

}
