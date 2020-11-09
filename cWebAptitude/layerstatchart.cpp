#include "layerstatchart.h"

int seuilClasseMinoritaire(2); // en dessous de ce seuil, les classes sont regroupées dans la catégorie "Autre"
layerStatChart::layerStatChart(Layer *aLay, std::map<std::string, int> aStat, OGRGeometry *poGeom):layerStat(aLay,aStat),mTable(NULL),rowAtMax(0),mGeom(poGeom)
{

    //std::cout << "création d'un layer StatChart pour " << mLay->getLegendLabel() << std::endl;
    mModel = std::make_shared<WStandardItemModel>();
    // pas sur que j'ai besoin de spécifier le proto
    //mModel->setItemPrototype(cpp14::make_unique<WStandardItem>());

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
        color col=mLay->getColor(kv.first);
        mModel->itemFromIndex(mModel->index(row,1))->setStyleClass(col.getStyleNameShort());

        if (kv.second>aMax) {aMax=kv.second; rowAtMax=row;}
        row++;
    }

}

Wt::WContainerWidget * layerStatChart::getChart(bool forRenderingInPdf){

    //std::cout << " creation d'un chart " << std::endl;
    Wt::WContainerWidget * aRes= new Wt::WContainerWidget();

    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV = aRes->setLayout(cpp14::make_unique<WVBoxLayout>());
    layoutV->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel(false)+"</h4>"));
    //aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    WContainerWidget * aCont = layoutV->addWidget(cpp14::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = aCont->setLayout(cpp14::make_unique<WHBoxLayout>());
    // ajout de la carte pour cette couche
    //layoutH->addWidget(cpp14::make_unique<olOneLay>(mLay,mGeom),0);
    staticMap sm(mLay,mGeom);
    // ça fonctionne mais je ne gère pas bien la taille de l'image dans le conteneur, pour l'instant la taille de l'image affichée est celle de l'image sur le disque

    if (forRenderingInPdf){
    Wt::WImage * im =layoutH->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLink()),0);
    im->resize(350,350);
    } else {
        Wt::WImage * im =layoutH->addWidget(cpp14::make_unique<Wt::WImage>(sm.getWLinkRel()),0);
        im->resize(350,350);
    }

    WContainerWidget * aContTableAndPie = layoutH->addWidget(cpp14::make_unique<WContainerWidget>());
    aContTableAndPie->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    //aContTableAndPie->setInline(0);
    aContTableAndPie->setOverflow(Wt::Overflow::Auto);

    WVBoxLayout * layoutV2 = aContTableAndPie->setLayout(cpp14::make_unique<WVBoxLayout>());

    //std::cout << " statsimple : " << mStatSimple.size() << " elem " << std::endl;
    if (!forRenderingInPdf){
    if (mStatSimple.size()>0){

        if (mTypeVar==TypeVar::Classe){
            WTableView* table =layoutV2->addWidget(cpp14::make_unique<WTableView>());
            //aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
            table->setMargin(10, Side::Top | Side::Bottom);
            table->setMargin(WLength::Auto, Side::Left | Side::Right);
            table->setAlternatingRowColors(0);
            table->setSortingEnabled(1,false);
            table->setSortingEnabled(0,false);// pas très utile
            //table->setAlternatingRowColors(true); // si je met à true , va overrider les couleurs que j'ai notée dans la colonne 3 du model qui sert de légende
            //std::cout << "set model " << std::endl;
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
        }
        if (mTypeVar==TypeVar::Continu){

            // pour MNH et MNT, pour l'instant  - recrée un vecteur stat, puis un model
            std::map<double, double> aStat;
            // je dois mettre et la hauteur en double, et le pct car sinon imprécision d'arrondi

            for (auto & kv : mStat){
                try {
                    double h(std::stod(kv.first));

                    if (mLay->getCode()=="MNH2019"){
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

            Chart::WCartesianChart *aChart = layoutH->addWidget(cpp14::make_unique<Chart::WCartesianChart>());
            //aChart->setBackground(WColor(220, 220, 220));
            aChart->setModel(model);
            aChart->setXSeriesColumn(0);
            aChart->setLegendEnabled(true);
            aChart->setType(Chart::ChartType::Scatter);

            auto s = cpp14::make_unique<Chart::WDataSeries>(1, Chart::SeriesType::Curve);
            s->setShadow(WShadow(3, 3, WColor(0, 0, 0, 127), 3));
            aChart->addSeries(std::move(s));

            aChart->resize(300, 300);    // WPaintedWidget must be given an explicit size.
            aChart->setMargin(20, Side::Top | Side::Bottom); // Add margin vertically.
            //aChart->setMargin(WLength::Auto, Side::Left | Side::Right); // Center horizontally. il faut mettre des marges, qui sont comtpée au départ du cammembert, pour mettre les label
            aChart->setMargin(50, Side::Left | Side::Right);
        }


    } else {
        layoutH->addWidget(cpp14::make_unique<WText>("Pas de statistique pour cette couche"));
    }
    }

    return aRes;
}

Wt::WContainerWidget * layerStatChart::getBarStat(){

Wt:WContainerWidget * aRes= new Wt::WContainerWidget();
    aRes->addWidget(cpp14::make_unique<batonnetApt>(this,mLay->Dico()->Dico_AptFull2AptAcro));

    return aRes;
}

void layerStat::simplifieStat(){

    mNbPix=0;
    for (auto & kv : mStat){
        mNbPix+=kv.second;
    }

    switch (mTypeVar){
    case TypeVar::Classe:{
        // calcul des pourcentages au lieu du nombre de pixel
        if (mNbPix>0){
            for (auto & kv : mStat){
                double pct = (100.0*kv.second)/mNbPix;
                kv.second=round(pct);
            }
        }

        int autres(0);
        int tot(0);
        for (auto & kv : mStat){
            if (kv.second>seuilClasseMinoritaire){
                mStatSimple.emplace(kv);
                tot+=kv.second;
            } else {
                autres+=kv.second;
            }
        }
        if (autres>0) {
            tot+=autres;
            // correction de l'erreur d'arrondi si elle est de 2 pct max
            if ((tot>97) & (tot <100)) { autres+= 100-tot; tot=100;}
            //std::cout << "ajout classe autre dans simplify stat " << autres << ", layer " << mLay->getLegendLabel() << std::endl;
            mStatSimple.emplace(std::make_pair("Autre",autres));
        }

        /* redondant, j'ajoute déjà des no data lors du calcul
        // ajout pct pour no data - certaine couche l'on déjà, d'autre pas.
        if (tot<97 & tot>5){
            mStatSimple.emplace(std::make_pair("Sans données",100-tot));
        }
        */
        break;}

    case TypeVar::Continu:{

        // pour l'instant, copie juste le mStat
        mStatSimple=mStat;

        // regroupe les valeurs en 10 groupes avec le mm nombre d'occurence
        // classer les occurences par valeur de hauteur car la map n'est pas trié par ordre de  hauteur car 11.0 et avant 2 ou 20.0, tri 'alphabétique' des chiffres.
        /*
        std::map<double, int> aStatOrdered;
        for (auto & kv : mStat){
            aStatOrdered.emplace(std::make_pair(std::stod(kv.first),kv.second));
        }


        int occurenceCumul(0);
        std::string curLimHaute("");
        double seuil(nbPix/nbClasse);
        int numClasCur(1);
        for (auto & kv : aStatOrdered){
            //std::cout << "hauteur " << kv.first << " , pixels " << kv.second << std::endl;
            if (kv.second!=0){
                if (kv.second<seuil){

                    occurenceCumul+=kv.second;
                    curLimHaute=nth_letter(numClasCur)+ " " + dToStr(kv.first);
                    //std::cout << " curLimHaute = " << curLimHaute << std::endl;

                } else {
                    if (occurenceCumul>0){
                        mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
                        numClasCur++;
                        mStatSimple.emplace(nth_letter(numClasCur)+ " " +dToStr(kv.first),(100*kv.second)/nbPix);
                        numClasCur++;
                        occurenceCumul=0;
                    } else {
                        mStatSimple.emplace(nth_letter(numClasCur)+ " " +dToStr(kv.first),(100*kv.second)/nbPix);
                    }
                }
                if (occurenceCumul>seuil){
                    mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
                    numClasCur++;
                    occurenceCumul=0;
                }
            }
        }

        if (occurenceCumul>0){
            mStatSimple.emplace(curLimHaute,(100*occurenceCumul)/nbPix);
        }
        */

        /*for (auto & kv : mStatSimple){
            std::cout << " limite haute : " << kv.first << " , " << kv.second << "%" << std::endl;

        }*/

        break;
    }
    }

}

int layerStat::getFieldVal(bool mergeOT){
    unsigned int aRes(0);
    if (mLay->Type()==TypeLayer::FEE || mLay->Type()==TypeLayer::CS){
        aRes=getO(mergeOT);
    } else if (mLay->getCode()=="MNH2019"){
        int nbPixTreeCover(0);
        for (auto & kv : mStat){
            try {
                if (std::stod(kv.first)>2.0){nbPixTreeCover+=kv.second;}
            }
            catch (const std::invalid_argument& ia) {
                // je retire le cout car sinon me pourris les logs
                //std::cerr << "layerStat::getFieldVal pour MNH 2019 - Invalid argument: " << ia.what() << '\n';
            }
        }
        aRes=(100*nbPixTreeCover)/mNbPix;
    } else if (mLay->getCode()=="MF"){
        // mStat ne contient pas la même chose si la couche est de type continu ou classe. pour Classe, c'est déjà des pcts
        for (auto & kv : mStat){
            //std::cout << kv.first << " nb pix " << kv.second << std::endl;
            if (kv.first=="Foret"){;aRes=kv.second;}
        }
        //aRes=(100*nbPixTreeCover)/mNbPix;
    } else {
        std::cout << "  pas de méthode pour remplir le champ de la table d'attribut pour le layer " << mLay->getLegendLabel() << std::endl;
    }
    return aRes;
}

std::string layerStat::getFieldValStr(){

    std::string aRes("");
    if (mLay->getCode()=="COMPO"){
        // on concatene toutes les essences
        for (auto & kv : mStat){
            //std::cout << "getFieldValStr kv.first " << kv.first << " kv.second " << kv.second << std::endl;
            // déjà sous forme de pct int pct=(100*kv.second)/mNbPix;
            if (kv.second>1){aRes+=getAbbreviation(kv.first)+":"+std::to_string(kv.second)+"% ";}
        }

    } else {
        std::cout << "  pas de méthode pour remplir le champ STRING de la table d'attribut pour le layer " << mLay->getLegendLabel() << std::endl;
    }
    return aRes;
}

std::string layerStat::summaryStat(){
    std::string aRes("");
    if (mLay->Var()==TypeVar::Classe){
        // on concatene toutes les essences
        for (auto & kv : mStat){
            if (kv.second>1){
                if (kv.second>99){ aRes+=kv.first;}
                aRes+=kv.first+": "+std::to_string(kv.second)+"% ";
            }
        }

    } else {
        std::cout << "  summaryStat sur une couche de variable de classes? " << mLay->getLegendLabel() << std::endl;
    }
    return aRes;

}

int layerStat::getO(bool mergeOT){
    unsigned int aRes(0);
    if (mLay->Type()==TypeLayer::FEE || mLay->Type()==TypeLayer::CS){
        // il faudrait plutôt faire le calcul sur les statistique mStat, car StatSimple peut avoir regroupé des classe trop peu représentées!
        // mais attention alors car le % n'est pas encore calculé, c'est le nombre de pixels.
        for (auto & kv : mStatSimple){
            int codeApt(666);
            for (auto & kv2 : *mLay->mDicoVal){
                if (kv2.second==kv.first){codeApt=kv2.first;}
            }
            int aptContr=mLay->Dico()->AptContraignante(codeApt);
            if (mergeOT) {if (aptContr<3) aRes+=kv.second;} else { if (aptContr<2) aRes+=kv.second; }
        }}
    else {
        std::cout << " problem, on me demande de calculer la proportion d'optimum pour une carte qui n'est pas une carte d'aptitude" << std::endl;
    }
    return aRes;
}

layerStat::layerStat(Layer * aLay, std::map<std::string,int> aStat):mLay(aLay),mStat(aStat),mTypeVar(aLay->Var()){
    simplifieStat();
}

olOneLay::olOneLay(Layer * aLay, OGRGeometry *poGeom):mLay(aLay){

    // export de la géométrie dans un fichier externe unique
    //char * c=;
    //char tmpfile[] =mLay->Dico()->File("TMPDIR")+"/geojson_XXXXXX";
    std::string name0 = std::tmpnam(nullptr);
    std::string name1 = name0.substr(5,name0.size()-5);
    std::string aOut = mLay->Dico()->File("TMPDIR")+"/"+name1;

    //std::string name1 = mktemp('geomXXXXX');
    //std::string name1 = "geomTMP.geojson";
    //std::string aOut=mLay->Dico()->File("TMPDIR")+"/"+name1;
    //std::cout << aOut << " fichier tmp " << std::endl;
    char * t=poGeom->exportToJson();
    std::ofstream ofs (aOut, std::ofstream::out);
    std::string a="{"
                  "'type': 'FeatureCollection',"
                  "'name': 'wt7fxhEi-epioux_sample',"
                  "'crs': { 'type': 'name', 'properties': { 'name': 'urn:ogc:def:crs:EPSG::31370' } },"
                  "'features': ["
                  "{ 'type': 'Feature', 'geometry':";
    boost::replace_all(a,"'","\"");
    ofs << a;
    ofs << t;
    ofs << "}]}";
    ofs.close();

    std::string JScommand;
    setWidth("40%");
    setMinimumSize(300,300);
    //setOverflow(Overflow::Visible);
    setOverflow(Overflow::Auto);
    std::string aFileIn(mLay->Dico()->File("staticMap"));
    std::ifstream in(aFileIn);
    std::stringstream ss;
    ss << in.rdbuf();
    in.close();
    JScommand=ss.str();
    boost::replace_all(JScommand,"MYTITLE",mLay->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",mLay->mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",mLay->mUrl);

    // remplacer l'identifiant du conteneur
    boost::replace_all(JScommand,"MYID",this->id());
    boost::replace_all(JScommand,"mapStat","mapStat"+this->id());

    // remplacer l'extent et le centre de la carte
    OGREnvelope ext;
    poGeom->getEnvelope(&ext);
    // agrandir un peu l'extend de la carte car sinon le polygone peut-être partiellement visible seulemement
    int bufX = (ext.MaxX-ext.MinX);
    int bufY = (ext.MaxY-ext.MinY);
    boost::replace_all(JScommand,"MAXX",std::to_string(ext.MaxX+bufX));
    boost::replace_all(JScommand,"MAXY",std::to_string(ext.MaxY+bufY));
    boost::replace_all(JScommand,"MINX",std::to_string(ext.MinX-bufX));
    boost::replace_all(JScommand,"MINY",std::to_string(ext.MinY-bufY));

    boost::replace_all(JScommand,"CENTERX",std::to_string(ext.MinX+((ext.MaxX-ext.MinX)/2)));
    boost::replace_all(JScommand,"CENTERY",std::to_string(ext.MinY+((ext.MaxY-ext.MinY)/2)));

    boost::replace_all(JScommand,"NAME","tmp/" + name1);
    //std::cout << JScommand << std::endl;
    this->doJavaScript(JScommand);
    //if (mLay->getCode()=="IGN"){std::cout << JScommand << std::endl;}
}

staticMap::staticMap(Layer * aLay, OGRGeometry *poGeom):mLay(aLay),mSx(700),mSy(700),ext(NULL){
    std::cout << "staticMap::staticMap" << std::endl;
    std::string name0 = std::tmpnam(nullptr);
    std::string name1 = name0.substr(5,name0.size()-5);
    mFileName = mLay->Dico()->File("TMPDIR")+"/"+name1+".png";
    // nécessaire pour Wlink
    mFileNameRel = "tmp/"+name1+".png";

    ext= new OGREnvelope;
    poGeom->getEnvelope(ext);
    // si la géométrie est un point, alors j'agrandi déjà de 40 mètres sinon division par 0== bug
    if (ext->MaxX-ext->MinX<10) {
    double bufPt(200);
    ext->MaxX+=bufPt;
    ext->MaxY+=bufPt;
    ext->MinX-=bufPt;
    ext->MinY-=bufPt;
    }
    // agrandir un peu l'extend de la carte car sinon le polygone peut-être partiellement visible seulemement
    int buf = (ext->MaxX-ext->MinX)/5;
    ext->MaxX+=buf;
    ext->MaxY+=buf;
    ext->MinX-=buf;
    ext->MinY-=buf;

    // taille de l'emprise de l'image
    mWx=ext->MaxX-ext->MinX;
    mWy=ext->MaxY-ext->MinY;
    // on rectifie la taille de l'image en pixel en fonction de la forme de la zone
    mSy=(double)mSx*(mWy/mWx);

    // d'abord transformer la couche wms en image locale, puis réouvrir et y dessiner le polygone
    if (mLay->wms2jpg(*ext,mSx,mSy,mFileName)) {

        //utiliser magick++ pour dessiner sur l'image
        //Magick::InitializeMagick("/usr/local/lib/");
        Magick::InitializeMagick(NULL);
        Magick::Image im(mFileName);

        //std::list<Magick::Drawable> drawList;

        drawList.push_back(Magick::DrawableStrokeColor("yellow"));
        drawList.push_back(Magick::DrawableStrokeWidth(3));
        drawList.push_back(Magick::DrawableFillColor("yellow"));
        //drawList.push_back(Magick::DrawableFillOpacity(0));
        drawList.push_back(Magick::DrawablePointSize(1.0));
        switch (poGeom->getGeometryType()){
        case (wkbPolygon):
        {
            OGRPolygon * pol =poGeom->toPolygon();
            drawPol(pol);
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
                    drawPol(pol);
                }
            }

            break;
        }
        // pour la carte générée pour analyse point, on ne dessine pas un polygone mais un cercle autour du point
        case wkbPoint:
        {
            OGRPoint * pt = poGeom->toPoint();
            // arg 3 et 4 pas intuitif, c'est la position ou doit se trouver le périmètre du cercle, pas du tout le radius ou autre
            drawList.push_back(Magick::DrawableCircle(xGeo2Im(pt->getX()),yGeo2Im(pt->getY()), xGeo2Im(pt->getX())-10,yGeo2Im(pt->getY())));

            break;
        }

        default:
            std::cout << "Geometrie " << poGeom->getGeometryName() << " non pris en charge " << std::endl;

            break;
        }

        // Draw everything using completed drawing list
        im.draw(drawList);
        drawList.clear();
        drawScaleLine(&im);

        im.write(mFileName);


    }
}

void staticMap::drawScaleLine(Magick::Image * im){

    drawList.push_back(Magick::DrawableStrokeColor("black"));
    drawList.push_back(Magick::DrawableStrokeWidth(4));
    drawList.push_back(Magick::DrawableFillColor("black"));
    drawList.push_back(Magick::DrawablePointSize(30.0));
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
    drawList.push_back(Magick::DrawableText(x0,y0,label));

    double y02= y0+40;
    int length= ((double)l*mSx)/mWx;
    drawList.push_back(Magick::DrawableLine(x0,y02,x0+length,y02));
    // les délimitation de la scalebar
    int a(10);
    drawList.push_back(Magick::DrawableLine(x0,y02-a,x0,y02+a));
    drawList.push_back(Magick::DrawableLine(x0+length,y02-a,x0+length,y02+a));
    im->draw(drawList);
}

void staticMap::drawPol(OGRPolygon * pol){
    OGRPoint ptTemp1, ptTemp2;
    pol->getExteriorRing()->getNumPoints();
    int NumberOfVertices = pol->getExteriorRing()->getNumPoints();
    for ( int k = 0; k < NumberOfVertices-1; k++ )
    {
        pol->getExteriorRing()->getPoint(k,&ptTemp1);
        pol->getExteriorRing()->getPoint(k+1,&ptTemp2);
        drawList.push_back(Magick::DrawableLine(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
    }
    // ajout segment final
    pol->getExteriorRing()->getPoint(0,&ptTemp1);
    pol->getExteriorRing()->getPoint(NumberOfVertices-1,&ptTemp2);
    drawList.push_back(Magick::DrawableLine(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));

    for (int c(0);c<pol->getNumInteriorRings();c++){
        int NumberOfVertices =pol->getInteriorRing(c)->getNumPoints();;
        for ( int k = 0; k < NumberOfVertices-1; k++ )
        {
            pol->getInteriorRing(c)->getPoint(k,&ptTemp1);
            pol->getInteriorRing(c)->getPoint(k+1,&ptTemp2);
            drawList.push_back(Magick::DrawableLine(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
        }
        // ajout segment final
        pol->getInteriorRing(c)->getPoint(0,&ptTemp1);
        pol->getInteriorRing(c)->getPoint(NumberOfVertices-1,&ptTemp2);
        drawList.push_back(Magick::DrawableLine(xGeo2Im(ptTemp1.getX()),yGeo2Im(ptTemp1.getY()),xGeo2Im(ptTemp2.getX()),yGeo2Im(ptTemp2.getY())));
    }
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

std::string getAbbreviation(std::string str)
{
    std::string aRes("");
    std::vector<std::string> words;
    std::string word("");
    for (auto x : str)
    {
        if (x == ' ' | x=='/')
        {
            word=removeAccents(word);// si je n'enlève pas les accents maintenant, les accents sont codé sur deux charachtère et en gardant les 2 premiers du mot je tronque l'accent en deux ce qui donne ququch d'illisible type ?
            if (word.size()>1){words.push_back(word);}
            //std::cout << "word is " << word << std::endl;
            word = "";
        }
        else
        {
            word = word + x;
        }
    }
    // pour le dernier mot :
    word=removeAccents(word);
    if (word.size()>1){words.push_back(word);}
    //std::cout << "word is " << word << std::endl;

    for (auto w : words){
        aRes+=w.substr(0,2);
    }
    //aRes=removeAccents(aRes);
    return aRes;
}

void batonnetApt::paintEvent(Wt::WPaintDevice *paintDevice){
    Wt::WPainter painter(paintDevice);
    int xcumul(0);
    for (auto & kv : mLayStat->StatSimple()){

        //std::cout << "batonnet Aptitude " << kv.first << " " << kv.second << std::endl;

        std::string aCodeStr(kv.first);
        color col(mLayStat->Lay()->getColor(aCodeStr));
        painter.setBrush(Wt::WBrush(Wt::WColor(col.mR,col.mG,col.mB)));
        double width=kv.second*(mW/100.0);
        painter.drawRect(xcumul, 0, width, mH);

        std::unique_ptr<Wt::WRectArea> rectPtr = Wt::cpp14::make_unique<Wt::WRectArea>(xcumul, 0, int(width), mH);
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

cDicoApt * layerStat::Dico(){return mLay->Dico();}
