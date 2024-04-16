#ifndef FORMVIELLECOUPERASE_H
#define FORMVIELLECOUPERASE_H
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WTextArea.h>
#include <Wt/WLabel.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WVBoxLayout.h>
#include <string>
#include <iostream>
#include <fstream>
#include <Wt/WPushButton.h>
#include <Wt/WEvent.h>
#include <Wt/WSignal.h>
#include <Wt/WJavaScriptSlot.h>
#include <Wt/WMessageBox.h>
#include "Wt/WTemplate.h"

#include <Wt/WItemDelegate.h>
#include <Wt/WStandardItem.h>
#include <Wt/WComboBox.h>
#include <Wt/Dbo/WtSqlTraits.h>
#include <Wt/WBootstrap5Theme.h>
#include <Wt/WLocalDateTime.h>
#include <cdicoapt.h>
#include <sqlite3.h>
#include <Wt/Mail/Message.h>
#include <Wt/Mail/Client.h>



using namespace Wt;
namespace dbo = Wt::Dbo;

class formVielleCoupeRase : public WApplication
{
public:
    formVielleCoupeRase(const Wt::WEnvironment& env, cDicoApt * dico, std::string aFileDB);

    void loadStyles();
    void submit();
    void sendSummaryMail();
    void displayLayer(std::string aCode);
    void displayCommune();
    OGREnvelope computeGlobalGeom(std::string aFile);
    void validDraw(std::string geojson);

    void vider(bool all=1);

    std::string format4SQL(std::string aString);


    WLineEdit *nomEncoderEdit_;
    WLineEdit *prenomEncoderEdit_;
    WLineEdit *contactEncoderEdit_;
    WCheckBox * keepInTouch;
    WLineEdit *ContactEdit_;
    WComboBox *anneeVCREdit_;
    WLineEdit *regeNatEdit_;
    WLineEdit *vegeBloquanteEdit_;
    WTextArea *VCRdescriptionEdit_;

    WLineEdit *vosrefEdit_;
    WComboBox *objectifEdit_;
    WLineEdit *spEdit_;
    WLineEdit *sanitEdit_;
    //WComboBox *itineraireEdit_;
    WLineEdit *travSylviEdit_;
    WLineEdit *plantationEdit_;
   // WLineEdit *hauteurEdit_;
    WLineEdit *gibierEdit_;

    WPushButton * bCancel;

    WComboBox * commune_;
    // clé ; index dans combobox. val= clé (INS commune , code Division)
    std::map<int,int> aMLabelCom;

private:
    double surf;
    bool polygValid;
    std::string polyg;
    std::string mBDFile;
    dbo::Session session;

    cDicoApt * mDico;
};

class Wol: public WContainerWidget
{
public:
    Wol();

    void updateView(){
        doJavaScript("refreshLayers();");
    }

    virtual void layoutSizeChanged(int width, int height)
    {
        WContainerWidget::layoutSizeChanged(width, height);
        doJavaScript("map.updateSize();");
    }

    JSignal<std::string>  polygGeojson_;
    JSignal<std::string>& polygGeojson() { return polygGeojson_; }
    JSlot slot;
};

#endif // FORMVIELLECOUPERASE_H
