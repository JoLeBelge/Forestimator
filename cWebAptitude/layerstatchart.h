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

class layerStat;
class layerStatChart;

// va contenir le titre, le tableau et le pie chart pour permettre une visualisation des statistiques calculé pour chacune des couches, typiquement les aptitudes des essences
class layerStat : public Wt::WContainerWidget
{
public:
    layerStat(Layer * aLay,std::map<std::string,int> aStat, std::string aMode);
    layerStat():mLay(NULL){} //constructeur vide
    void simplifieStat();
    int getO(bool mergeOT=false);// proportion en optimum
protected:
    Layer * mLay;
    std::map<std::string, int> mStat;
    std::map<std::string, int> mStatSimple;
    std::string mMode; // fee vs cs
};

class layerStatChart : public layerStat
{
public:
    layerStatChart(Layer * aLay,std::map<std::string,int> aStat, std::string aMode);
    layerStatChart():mLay(NULL),mTable(NULL),mChart(NULL){} //constructeur vide
    Wt::WContainerWidget * getChart();
    Chart::WPieChart * mChart;
private:
    std::shared_ptr<WStandardItemModel> mModel;
    Layer * mLay;
    //WStandardItemModel * mModel;
    WTableView * mTable;
    std::map<std::string, int> mStat;
    std::map<std::string, int> mStatSimple;
    int rowAtMax;
    std::string mMode; // fee vs cs

};

#endif // LAYERSTATCHART_H
