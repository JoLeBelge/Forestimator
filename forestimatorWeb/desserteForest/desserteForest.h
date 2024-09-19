#ifndef DF_H
#define DF_H
#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WString.h>
#include <Wt/WBootstrapTheme.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WTableRow.h>
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
#include "layerstatchart.h"
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
#include "Wt/WDoubleValidator.h"
#include "Wt/WValidator.h"
#include <Wt/Dbo/Dbo.h>

#include "ACR/formviellecouperase.h"

using namespace Wt;
namespace dbo = Wt::Dbo;

class formDesserteForest : public WApplication
{
public:
    formDesserteForest(const Wt::WEnvironment& env, cDicoApt * dico, std::string aFileDB);

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
    WLineEdit *contactEncoderGSMEdit_;
    WCheckBox * keepInTouch;
    WComboBox *typeContactEdit_;
    WComboBox *anneeVCREdit_;
    WLineEdit *regeNatEdit_;
    WLineEdit *vegeBloquanteEdit_;
    WTextArea *VCRdescriptionEdit_;

    WLineEdit *vosrefEdit_;
    WComboBox *objectifEdit_;
    WLineEdit *spEdit_;
    WLineEdit *sanitEdit_;
    WLineEdit *travSylviEdit_;
    WLineEdit *plantationEdit_;
    WLineEdit *gibierEdit_;

    WPushButton * bCancel;

    WComboBox * commune_;
    // clé ; index dans combobox. val= clé (INS commune , code Division)
    std::map<int,int> aMLabelCom;

private:

    std::string SQLstring;

    double surf;
    bool polygValid;
    std::string polyg;
    OGRGeometry * geom;
    std::string mBDFile;
    dbo::Session session;

    cDicoApt * mDico;
};

#endif // DF_H
