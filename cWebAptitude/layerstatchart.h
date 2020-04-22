#ifndef LAYERSTATCHART_H
#define LAYERSTATCHART_H
#include <Wt/Chart/WPieChart.h>
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WEnvironment.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WTableView.h>
#include "layer.h"
using namespace Wt;
using namespace Wt::Chart;

// va contenir le titre, le tableau et le pie chart pour permettre une visualisation des statistiques calculé pour chacune des couches, typiquement les aptitudes des essences
class layerStatChart : public Wt::WContainerWidget
{
public:
    layerStatChart(Layer * aLay,std::map<std::string,int> aStat);
    layerStatChart():mLay(NULL),mTable(NULL),mChart(NULL){} //constructeur vide
    Wt::WContainerWidget * getChart();
    Chart::WPieChart * mChart;
    void simplifieStat();
private:
    std::shared_ptr<WStandardItemModel> mModel;
    Layer * mLay;
    //WStandardItemModel * mModel;
    WTableView * mTable;
    std::map<std::string, int> mStat;
    int rowAtMax;

};

#endif // LAYERSTATCHART_H
