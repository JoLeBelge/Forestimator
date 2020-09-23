#ifndef STATWINDOW_H
#define STATWINDOW_H
#include "grouplayers.h"
#include "layerstatchart.h"
#include <hpdf.h>
#include <Wt/WResource.h>
#include <Wt/Http/Request.h>
#include <Wt/Http/Response.h>
#include <Wt/Render/WPdfRenderer.h>
#include <Wt/WApplication.h>
//#include "iostream"

class ReportResource;
class statWindow;

namespace {
    void HPDF_STDCALL error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no,
               void *user_data) {
    fprintf(stderr, "libharu error: error_no=%04X, detail_no=%d\n",
        (unsigned int) error_no, (int) detail_no);
    }
}


class statWindow : public Wt::WContainerWidget
{
public:
    // a besoin du dictionnaire pour créer le layer qui contient la carte IGN pour faire la carte de situation globale
    statWindow(cDicoApt * aDico);

    void vider();
    void titre(std::string aTitre){mTitre->setText(aTitre);}
    void add1Aptitude(layerStatChart * lstat);
    void add1layerStat(Wt::WContainerWidget * layerStat);
    void generateGenCarte(OGRFeature *poFeature);

    void export2pdf();

    WText * mTitre;
    WTable * mAptTable;
    WPushButton * createPdfBut;
    WContainerWidget * mCarteGenCont;
private:
    cDicoApt * mDico;

    Wt::WVBoxLayout * layout;



    std::vector<WContainerWidget *> mVContStatIndiv;
    Layer * mIGN;

};



class ReportResource : public Wt::WResource
{
public:
    ReportResource(statWindow * aSW)
        : WResource()
    {
    suggestFileName("report.pdf");

    std::ostringstream o;

    if (aSW->mAptTable!=NULL) {aSW->mAptTable->htmlText(o);}

    strHTML = o.str();
    //aSW->mTitre->text();
    //strHTML=aSW->mTitre->text().toUTF8();
    //strHTML=Wt::WText::tr("page_presentation").toUTF8();
    std::cout <<  strHTML << std::endl;

    }

    virtual ~ReportResource()
    {
    beingDeleted();
    }

    virtual void handleRequest(const Wt::Http::Request& request,
                               Wt::Http::Response& response)
    {

    std::cout << " handleRequest of reportRessource " << std::endl;
    response.setMimeType("application/pdf");

    HPDF_Doc pdf = HPDF_New(error_handler, 0);

    // Note: UTF-8 encoding (for TrueType fonts) is only available since libharu 2.3.0 !
    HPDF_UseUTFEncodings(pdf);

    renderReport(pdf);

    HPDF_SaveToStream(pdf);
    unsigned int size = HPDF_GetStreamSize(pdf);
    HPDF_BYTE *buf = new HPDF_BYTE[size];
    HPDF_ReadFromStream (pdf, buf, &size);
    HPDF_Free(pdf);
    response.out().write((char*)buf, size);
    delete[] buf;
    }

private:
    std::string strHTML;

    void renderReport(HPDF_Doc pdf) {
        renderPdf(Wt::WString(strHTML), pdf);
        //renderPdf(Wt::WText::tr("page_presentation"),pdf);
    }

    void renderPdf(const Wt::WString& html, HPDF_Doc pdf)
    {
    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    Wt::Render::WPdfRenderer renderer(pdf, page);
    renderer.setMargin(2.54);
    renderer.setDpi(96);
    renderer.render(html);
    }
};


#endif // STATWINDOW_H
