#include "layerstatchart.h"

layerStatChart::layerStatChart(Layer *aLay, std::map<std::string, int> aStat):mLay(aLay),mTable(NULL),mStat(aStat),rowAtMax(0)
{

    simplifieStat();
    //std::cout << "création d'un layer StatChart pour " << mLay->getLegendLabel() << std::endl;
    mModel = std::make_shared<WStandardItemModel>();
    // pas sur que j'ai besoin de spécifier le proto
    //mModel->setItemPrototype(cpp14::make_unique<WStandardItem>());

    // Configure the header.
    mModel->insertColumns(mModel->columnCount(), 2);
    //mModel->setHeaderData(0, WString(mLay->getLegendLabel()));
    mModel->setHeaderData(0, "Catégories");
    mModel->setHeaderData(1, WString("Proportions"));

    // Set data in the model.
    mModel->insertRows(mModel->rowCount(), mStat.size());
    int row = 0;
    int aMax(1);

    //std::cout << "set data in model" << std::endl;
    for (auto & kv : mStat){
        //clé : la valeur au format légende (ex ; Optimum). Valeur ; pourcentage pour ce polygone
        mModel->setData(  row, 0, WString(kv.first));
        //mModel->setData(  row, 1, WString("Blueberry"), ItemDataRole::ToolTip);
        mModel->setData(  row, 1, kv.second);
        if (kv.second>aMax) {aMax=kv.second; rowAtMax=row;}
        row++;
    }

}

Wt::WContainerWidget * layerStatChart::getChart(){
    // crée un smart ptr pour un chart vide
    Wt:WContainerWidget * aRes= new Wt::WContainerWidget();
    aRes->setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Center);
    aRes->setInline(0);
    aRes->setOverflow(Wt::Overflow::Auto);

    aRes->addWidget(cpp14::make_unique<WText>("<h4>"+mLay->getLegendLabel()+"</h4>"));
    aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    if (mStat.size()>0){
    WTableView* table =aRes->addWidget(cpp14::make_unique<WTableView>());
    aRes->addWidget(Wt::cpp14::make_unique<Wt::WBreak>());
    table->setMargin(10, Side::Top | Side::Bottom);
    table->setMargin(WLength::Auto, Side::Left | Side::Right);
    table->setSortingEnabled(1,false);
    table->setSortingEnabled(0,false);// pas très utile
    table->setModel(mModel);
    table->setColumnWidth(0, 200);
    table->setColumnWidth(1, 150);
    table->setRowHeight(28);
    table->setHeaderHeight(28);
    table->setWidth(200 + 150 + 14+2);

    Chart::WPieChart * aChart  =aRes->addWidget(cpp14::make_unique<Chart::WPieChart>());
    aChart->setModel(mModel);       // Set the model.
    aChart->setLabelsColumn(0);    // Set the column that holds the labels.
    aChart->setDataColumn(1);      // Set the column that holds the data.

    // changer la couleur
    //std::cout << "change la couleur" << std::endl;
    int row = 0;
    for (auto & kv : mStat){
          std::string aCodeStr(kv.first);
          //std::cout << " row " <<  row << ", " << aCodeStr<< std::endl;
        color col(mLay->getColor(aCodeStr));
       // std::cout << "got color" << std::endl;
        aChart->setBrush(row,Wt::WBrush(Wt::WColor(col.mR,col.mG,col.mB)));
        //std::cout << "brush setted " << std::endl;
        row++;
    }
    //std::cout << "config la position des labels" << std::endl;
    // Configure location and type of labels.
   //aChart->setDisplayLabels(Chart::LabelOption::Outside |
                            Chart::LabelOption::TextLabel |
                            Chart::LabelOption::TextPercentage);
    // Enable a 3D and shadow effect.
    aChart->setPerspectiveEnabled(true, 0.2);
    aChart->setShadowEnabled(true);

    //if (mStat.size()>1) {aChart->setExplode(rowAtMax, 0.1);}  // Explode l'élément majoritaire WARN, le camembert sort du graphique, bug
    aChart->resize(300, 300);    // WPaintedWidget must be given an explicit size.
    aChart->setMargin(20, Side::Top | Side::Bottom); // Add margin vertically.
    //aChart->setMargin(WLength::Auto, Side::Left | Side::Right); // Center horizontally. il faut mettre des marges, qui sont comtpée au départ du cammembert, pour mettre les label
    aChart->setMargin(50, Side::Left | Side::Right);
    } else {
      aRes->addWidget(cpp14::make_unique<WText>("Pas de statistique pour cette couche"));
    }

    return aRes;
}

void layerStatChart::simplifieStat(){
    std::map<std::string, int> aStatSimple;
    int autres(0);
    int tot(0);
    for (auto & kv : mStat){
       if (kv.second>5){
           aStatSimple.emplace(kv);
           tot+=kv.second;
       } else {
           autres+=kv.second;
       }
    }
    if (autres>0) {

        tot+=autres;
        // correction de l'erreur d'arrondi si elle est de 2 pct max
        if (tot>97 & tot <100) { autres+= 100-tot; tot=100;}
        aStatSimple.emplace(std::make_pair("Autre",autres));
    }
    // ajout pct pour no data - certaine couche l'on déjà, d'autre pas.
    if (tot<97 & tot>5){
       aStatSimple.emplace(std::make_pair("Sans données",100-tot));
    }

    mStat=aStatSimple;
}

