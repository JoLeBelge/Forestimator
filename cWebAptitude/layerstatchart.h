#ifndef LAYERSTATCHART_H
#define LAYERSTATCHART_H
#include <Wt/Chart/WPieChart.h>
#include <Wt/Chart/WCartesianChart.h>
#include <Wt/WItemDelegate.h>
#include <Wt/Chart/WDataSeries.h>
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WEnvironment.h>
#include <Wt/WStandardItemModel.h>
#include <Wt/WStandardItem.h>
#include <Wt/WTableView.h>
#include "layer.h"
using namespace Wt;
using namespace Wt::Chart;



#include <iomanip>

std::string dToStr(double d);
std::string nth_letter(int n);

class layerStat;
class layerStatChart;

// va contenir le titre, le tableau et le pie chart pour permettre une visualisation des statistiques calculé pour chacune des couches, typiquement les aptitudes des essences
class layerStat : public Wt::WContainerWidget
{
public:
    layerStat(Layer * aLay,std::map<std::string,int> aStat, std::string aMode);
    layerStat(const layerStat &ls){
        std::cout << "construct by copy layerStat " << std::endl;
        mLay=ls.mLay;
        mStat=ls.mStat;
        mStatSimple=ls.mStatSimple;
        mMode=ls.mMode;
        mTypeVar=ls.mTypeVar;
    }

    void simplifieStat();
    int getO(bool mergeOT=false);// proportion en optimum

    int getFieldVal(bool mergeOT=false);
protected:
    Layer * mLay;
    // key ; classe ou valeur, val ; nombre d'occurence
    std::map<std::string, int> mStat;
    std::map<std::string, int> mStatSimple;
    std::string mMode; // fee vs cs
    TypeVar mTypeVar; // pour distinguer le type de variable, continue (MNH) ou classes (aptitude)
};

class layerStatChart : public layerStat
{
public:
    layerStatChart(Layer * aLay,std::map<std::string,int> aStat, std::string aMode);
    layerStatChart(const layerStatChart &ls):layerStat(ls){
        std::cout << "construct by copy layerStatChart " << std::endl;
        mModel=ls.mModel;
        rowAtMax=ls.rowAtMax;
        mTable=ls.mTable;
        mChart=ls.mChart;
    }
    Wt::WContainerWidget * getChart();
    Chart::WPieChart * mChart;
    bool deserveChart(){return mStatSimple.size()>0;}
private:
    std::shared_ptr<WStandardItemModel> mModel;
    //Layer * mLay;
    //WStandardItemModel * mModel;
    WTableView * mTable;
    //std::map<std::string, int> mStat;
    //std::map<std::string, int> mStatSimple;
    int rowAtMax;
    //std::string mMode; // fee vs cs

};

#endif // LAYERSTATCHART_H
