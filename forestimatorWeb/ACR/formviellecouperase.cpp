#include "formviellecouperase.h"

extern bool globTest;

formVielleCoupeRase::formVielleCoupeRase(const WEnvironment &env, cDicoApt *dico, std::string aFileDB) : Wt::WApplication(env),
    session(),mDico(dico),polygValid(0),mBDFile(aFileDB),keepInTouch(0)
{
    loadStyles();
    WLabel *label;

    messageResourceBundle().use(docRoot() + "/encodageVCR");
    setTitle(WString("Suivi Coupe Rase"));
    Wt::WTemplate * tpl = root()->addWidget(cpp14::make_unique<Wt::WTemplate>(WString::tr("template")));
    WContainerWidget * cont = tpl->bindWidget("contTitre", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("titre"));

    cont = tpl->bindWidget("contContact", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionEncoder"));

    WTable * table= cont->addNew<WTable>();
    int row=0;

    nomEncoderEdit_ = table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    table->elementAt(row,1)->setMinimumSize("300px", "300px");
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("nom")));
    label->setBuddy(nomEncoderEdit_);
    nomEncoderEdit_->setPlaceholderText(WString::tr("nom.ph"));
    ++row;
    prenomEncoderEdit_ = table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("prenom")));
    label->setBuddy(prenomEncoderEdit_);
    prenomEncoderEdit_->setPlaceholderText(WString::tr("prenom.ph"));
    ++row;
    contactEncoderEdit_= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contact")));
    contactEncoderEdit_->setPlaceholderText(WString::tr("contact.ph"));
    contactEncoderEdit_->setValidator(std::make_shared<WValidator>(true));
    contactEncoderEdit_->validator()->setInvalidBlankText("Nous avons besoin de pouvoir vous contacter");
    contactEncoderEdit_->addStyleClass("textEdit");
    ++row;
    contactEncoderGSMEdit_= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contact.gsm")));
    contactEncoderGSMEdit_->setPlaceholderText(WString::tr("contact.gsm.ph"));
    ++row;
    keepInTouch= table->elementAt(row,1)->addWidget(std::make_unique<WCheckBox>());
    keepInTouch->setMinimumSize("30px","30px");
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("kit")));
    ++row;
    //ContactEdit_= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    typeContactEdit_= table->elementAt(row,1)->addWidget(std::make_unique<WComboBox>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("typeEncoder")));
    //ContactEdit_->setPlaceholderText(WString::tr("contactACR.ph"));
    //ContactEdit_->addStyleClass("textEdit");
    typeContactEdit_->addItem(WString::tr("typeEncoder0"));
    typeContactEdit_->addItem(WString::tr("typeEncoder1"));
    typeContactEdit_->addItem(WString::tr("typeEncoder2"));
    typeContactEdit_->addItem(WString::tr("typeEncoder3"));
    typeContactEdit_->addItem(WString::tr("typeEncoder4"));
    typeContactEdit_->addItem(WString::tr("typeEncoder5"));

    cont = tpl->bindWidget("contACR1", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionVCR1"));
    table= cont->addNew<WTable>();
    table->addStyleClass("table-encodage");
    row=0;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("anneeVCR")));
    ++row;
    anneeVCREdit_= table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
    anneeVCREdit_->addItem(WString::tr("anneeCoupe0"));
    anneeVCREdit_->addItem(WString::tr("anneeCoupe1"));
    anneeVCREdit_->addItem(WString::tr("anneeCoupe2"));
    anneeVCREdit_->addItem(WString::tr("anneeCoupe3"));
    anneeVCREdit_->addItem(WString::tr("anneeCoupe4"));
    //++row;

    cont = tpl->bindWidget("contACR2", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionVCR2"));
    table= cont->addNew<WTable>();
    table->addStyleClass("table-encodage");
    row=0;

    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("vosRef")));
    ++row;
    vosrefEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    vosrefEdit_->setPlaceholderText(WString::tr("vosRef.ph"));
    row++;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("objectif")));
    objectifEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
    objectifEdit_->addItem(WString::tr("objectif0"));
    objectifEdit_->addItem(WString::tr("objectif1"));
    objectifEdit_->addItem(WString::tr("objectif2"));
    objectifEdit_->addItem(WString::tr("objectif3"));

    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("sp")));
    ++row;
    spEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    spEdit_->setPlaceholderText(WString::tr("sp.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("sanit")));
    ++row;
    sanitEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    sanitEdit_->setPlaceholderText(WString::tr("sanit.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("regeNat")));
    ++row;
    regeNatEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    //regeNatEdit_->setMinimumSize("200px", "100%");
    regeNatEdit_->setPlaceholderText(WString::tr("regeNat.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("vegeBloquante")));
    ++row;
    vegeBloquanteEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    //vegeBloquanteEdit_->setMinimumSize("200px", "100%");
    vegeBloquanteEdit_->setPlaceholderText(WString::tr("vegeBloquante.ph"));
    /*++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("itineraire")));
    ++row;
    itineraireEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
    itineraireEdit_->addItem(WString::tr("itineraire0"));
    itineraireEdit_->addItem(WString::tr("itineraire1"));
    itineraireEdit_->addItem(WString::tr("itineraire2"));
    itineraireEdit_->addItem(WString::tr("itineraire3"));
    */
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("trav.sylvi")));
    ++row;
    travSylviEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    travSylviEdit_->setPlaceholderText(WString::tr("trav.sylvi.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("plantation")));
    ++row;
    plantationEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    plantationEdit_->setPlaceholderText(WString::tr("plantation.ph"));
    //++row;
    /*label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("hauteur")));
    ++row;
    hauteurEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    hauteurEdit_->setPlaceholderText(WString::tr("hauteur.ph"));
    */
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("gibier")));
    ++row;
    gibierEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    gibierEdit_->setPlaceholderText(WString::tr("gibier.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("descriptionVCR")));
    ++row;
    VCRdescriptionEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WTextArea>());
    VCRdescriptionEdit_->setColumns(40);
    VCRdescriptionEdit_->setRows(5);
    VCRdescriptionEdit_->setPlaceholderText(WString::tr("descriptionVCR.ph"));

    nomEncoderEdit_->enterPressed().connect(prenomEncoderEdit_, &WWidget::setFocus);
    prenomEncoderEdit_->enterPressed().connect(contactEncoderEdit_, &WWidget::setFocus);
    contactEncoderEdit_->enterPressed().connect(contactEncoderGSMEdit_, &WWidget::setFocus);
    contactEncoderGSMEdit_->enterPressed().connect(keepInTouch, &WWidget::setFocus);
    keepInTouch->enterPressed().connect(typeContactEdit_, &WWidget::setFocus);

    anneeVCREdit_->changed().connect(vosrefEdit_, &WWidget::setFocus);
    vosrefEdit_->enterPressed().connect(objectifEdit_, &WWidget::setFocus);
    objectifEdit_->changed().connect(spEdit_, &WWidget::setFocus);
    spEdit_->enterPressed().connect(sanitEdit_, &WWidget::setFocus);
    sanitEdit_->enterPressed().connect(regeNatEdit_, &WWidget::setFocus);
    regeNatEdit_->enterPressed().connect(vegeBloquanteEdit_, &WWidget::setFocus);
    vegeBloquanteEdit_->enterPressed().connect(travSylviEdit_, &WWidget::setFocus);
    travSylviEdit_->enterPressed().connect(plantationEdit_, &WWidget::setFocus);
    plantationEdit_->enterPressed().connect(gibierEdit_, &WWidget::setFocus);
    gibierEdit_->enterPressed().connect(VCRdescriptionEdit_, &WWidget::setFocus);

    cont = tpl->bindWidget("contLoca", std::make_unique<WContainerWidget>());
    cont->addStyleClass("encodage");

    WVBoxLayout * la = cont->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    cont= la->addWidget(std::make_unique<WContainerWidget>());
    cont->addWidget(std::make_unique<Wt::WText>(WString::tr("titreLocalisation")));
    cont= la->addWidget(std::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = cont->setLayout(Wt::cpp14::make_unique<Wt::WHBoxLayout>());
    auto smart_map = std::make_unique<Wol>();
    map = smart_map.get();

    map->polygGeojson().connect(std::bind(&formVielleCoupeRase::validDraw,this, std::placeholders::_1));

    std::ifstream t(mDico->File("docroot")+"/js/initOL_vcr.js");
    std::stringstream ss;
    ss << t.rdbuf();
    //std::cout << "initialize map wol " << std::endl;
    doJavaScript(ss.str());
    layoutH->addWidget(std::move(smart_map));

    cont= layoutH->addWidget(std::make_unique<WContainerWidget>());

    cont->addNew<Wt::WText>(WString::tr("titreCommune"));
    commune_ = cont->addWidget(std::make_unique<WComboBox>());
    aMLabelCom.emplace(std::make_pair(0,0));
    commune_->addItem(WString::tr("cadastre.choisir"));
    int c(1);
    for (auto & l : mDico->mCadastre->getCommuneLabel()){
        // sert à avoir directement le lien entre l'index de la combobox commune et le code postal
        aMLabelCom.emplace(std::make_pair(c,l.first));
        commune_->addItem(l.second);
        c++;
    }
    commune_->changed().connect(std::bind(&formVielleCoupeRase::displayCommune, this));

    cont->addNew<Wt::WText>(WString::tr("titreCatalogue"));
    std::vector<std::string> vLs={"IGN", "ortho2020", "ortho2023", "Cadastre"};
    for (std::string code : vLs){
        std::shared_ptr<layerBase> aL1=mDico->getLayerBase(code);
        WPushButton * but = cont->addNew<Wt::WPushButton>(aL1->Nom());
        but->addStyleClass("mybtn2");
        //WText * wtext1= cont->addNew<Wt::WText>(aL1->Nom());
        //wtext1->addStyleClass("encodageLayer");
        //wtext1->decorationStyle().setCursor(Cursor::PointingHand);
        cont->addNew<WBreak>();
        but->clicked().connect([=] {this->displayLayer(aL1->Code());});
    }
    cont->addNew<WBreak>();
    cont->addNew<Wt::WText>(WString::tr("btnPolyg"));
    bCancel =cont->addNew<WPushButton>(WString::tr("initPol"));
    bCancel->setMinimumSize("100%"," ");
    bCancel->addStyleClass("btn btn-danger");
    bCancel->clicked().connect([=] {doJavaScript("acr_src.clear();map.addInteraction(draw);"); polygValid=0;});
    WPushButton * bUndo =cont->addNew<WPushButton>(WString::tr("cancelPoint"));
    bUndo->setMinimumSize("100%"," ");
    bUndo->clicked().connect([=] {doJavaScript("draw.removeLastPoint();");});


    WPushButton * bSubmit = tpl->bindWidget("bsubmit", std::make_unique<WPushButton>(WString::tr("submit")));
    bSubmit->clicked().connect([=] {submit();});
}

void formVielleCoupeRase::submit(){

    if (contactEncoderEdit_->validate()!=ValidationState::Valid){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Votre contact",
                                                          "Renseignez-nous un moyen pour vous contacter svp",
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();

    } else if(!polygValid){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Emplacement de la coupe rase",
                                                          "Veillez localiser la coupe rase et dessiner le contour de la parcelle sur la carte"
                                                          ,
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();
    } else if(anneeVCREdit_->currentIndex()==0){
        Wt::WMessageBox * messageBox = this->addChild(std::make_unique<Wt::WMessageBox>(
                                                          "Anciennetée de la coupe rase",
                                                          "Veillez nous renseigner l'anciennetée de la coupe rase (menu déroulant)"
                                                          ,
                                                          Wt::Icon::Information,
                                                          Wt::StandardButton::Ok));
        messageBox->setModal(true);
        messageBox->buttonClicked().connect([=] {
            this->removeChild(messageBox);
        });
        messageBox->show();

    } else {
        // sauver la coupe rase dans la BD
        int rc;
        sqlite3 *db_;
        rc = sqlite3_open(mBDFile.c_str(), &db_);
        if( rc )  {} else {

            WLocalDateTime d= WLocalDateTime::currentDateTime();
            //std::cout << " sauve la coupe rase " << std::endl;
            sqlite3_stmt * stmt;
            SQLstring="INSERT INTO acr (date,vosRef,nom,prenom,contact,gsm,keepInTouch,typeContact,anneeCoupe,regeNat,vegeBloquante,objectif,spCoupe,sanitCoupe,travaux,plantation,gibier,descr,surf,polygon) VALUES ('"
                    +d.toString().toUTF8()+"',"
                    +"'"+format4SQL(vosrefEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(nomEncoderEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(prenomEncoderEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(contactEncoderEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(contactEncoderGSMEdit_->valueText().toUTF8())+"',"
                    +std::to_string(keepInTouch->isChecked())+","
                    +"'"+format4SQL(typeContactEdit_->currentText().toUTF8())+"',"
                    +"'"+format4SQL(anneeVCREdit_->currentText().toUTF8())+"',"
                    +"'"+format4SQL(regeNatEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(vegeBloquanteEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(objectifEdit_->currentText().toUTF8())+"',"
                    +"'"+format4SQL(spEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(sanitEdit_->valueText().toUTF8())+"',"
                    //+"'"+format4SQL(itineraireEdit_->currentText().toUTF8())+"',"
                    +"'"+format4SQL(travSylviEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(plantationEdit_->valueText().toUTF8())+"',"
                    //+"'"+format4SQL(hauteurEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(gibierEdit_->valueText().toUTF8())+"',"
                    +"'"+format4SQL(VCRdescriptionEdit_->valueText().toUTF8())+"',"
                    +std::to_string(surf)+","
                    +"'"+polyg+"');";
            if (globTest){ std::cout << "sql : " << SQLstring << std::endl;}
            sqlite3_prepare_v2(db_, SQLstring.c_str(), -1, &stmt, NULL );
            // applique l'update
            sqlite3_step( stmt );
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db_);


        sendSummaryMail();

        Wt::StandardButton answer = Wt::WMessageBox::show( WString::tr("saveACR.msg.titre"),
                                                           WString::tr("saveACR.msg.cont"),
                                                           Wt::StandardButton::Yes | Wt::StandardButton::No);
        if (answer == Wt::StandardButton::Yes){
            vider(0);
        } else {
            vider();
        }


    }
}

std::string formVielleCoupeRase::format4SQL(std::string aString){

    boost::replace_all(aString,"'","\"");
    return aString;
}

void formVielleCoupeRase::vider(bool all){
    //std::cout << "vider formulaire"<< std::endl;
    if(all){
        nomEncoderEdit_->setText("");
        prenomEncoderEdit_->setText("");
        contactEncoderEdit_->setText("");
        contactEncoderGSMEdit_->setText("");
        keepInTouch->setChecked(0);
        typeContactEdit_->setCurrentIndex(0);
    }
    anneeVCREdit_->setCurrentIndex(0);
    regeNatEdit_->setText("");
    vegeBloquanteEdit_->setText("");
    VCRdescriptionEdit_->setText("");
    vosrefEdit_->setText("");
    objectifEdit_->setCurrentIndex(0);
    spEdit_->setText("");
    sanitEdit_->setText("");
    travSylviEdit_->setText("");
    plantationEdit_->setText("");
    gibierEdit_->setText("");
    polygValid=0;
    polyg="";
    doJavaScript("acr_src.clear();map.addInteraction(draw);");
    doJavaScript("document.body.scrollTop = 0; // For Safari");
    doJavaScript("document.documentElement.scrollTop = 0; // For Chrome, Firefox, IE and Opera");
}

void formVielleCoupeRase::loadStyles(){
    std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);
    useStyleSheet("resources/themes/default/wt.css");
    useStyleSheet("style/style.css");
    // init the OpenLayers javascript api
    require("jslib/v9.0.0-dist/ol.js");
    useStyleSheet("jslib/v9.0.0-dist/ol.css");
    require("jslib/proj4js-2.6.1/dist/proj4.js");
    require("jslib/proj4js-2.6.1/dist/proj4-src.js");
    enableUpdates();
}

Wol::Wol():slot(this),polygGeojson_(this,"toto")
{
    setPadding(0);
    setMargin(0);
    setLayoutSizeAware(1);
    setWidth("70%");
    setMinimumSize(500,600);
    setOverflow(Overflow::Visible);
    setId("map");

    setToolTip(Wt::WString::tr("map.tooltip"));

    // à chaque click le slot est appellé, et quand un polygone est fini, ça déclenche sa validation
    slot.setJavaScript
            ("function (){"
             "var str = writer.writeFeatures(acr_src.getFeatures());"
             "if (acr_src.getFeatures().length != 0) {"
             + polygGeojson_.createCall({"str"}) +
             "}}"
             );
    // attention, ne se déclenche pas si clic droit de la souris!
    this->clicked().connect(this->slot);
    // ça c'est plus safe meme si du coup ça fait beaucoup travailler pour rien.
    this->mouseWentOut().connect(this->slot);
}

void formVielleCoupeRase::displayLayer(std::string aCode) {
    std::shared_ptr<layerBase>l= mDico->getLayerBase(aCode);
    std::string JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
            "extent: extent,"+
            "title: 'MYTITLE',"+
            "source: new ol.source.TileWMS({"+
            "preload: Infinity,"+
            "title: 'MYTITLE',"+
            "url: 'MYURL',"+
            "crossOrigin: 'null',"+
            "attributions: 'MYATTRIBUTION',"+
            "params: {"+
            "'LAYERS': 'MYLAYER',"+
            "'TILED': false,"+
            //'TILED': false, // avant était à true mais ça faisait bugger cartoweb_topo
            "'FORMAT': 'image/png'"+
            "},"+
            "tileGrid: tileGrid,"+
            "projection: 'EPSG:31370',"+
            " }),"+
            " opacity: 1"+
            "});";

    if (l->mTypeWMS==TypeWMS::ArcGisRest){
        JScommand=std::string("activeLayer  = new ol.layer.Tile({")+
                "extent: extent,"+
                "title: 'MYTITLE',"+
                "  source: new ol.source.TileArcGISRest({"+
                "    attributions: 'MYATTRIBUTION',"+
                " url:'MYURL'"+
                "}),"+
                " opacity: 1"+
                "});";

    }

    JScommand+="activeLayers['MYCODE'] = activeLayer;updateGroupeLayers();";
    boost::replace_all(JScommand,"MYTITLE",l->getLegendLabel());
    boost::replace_all(JScommand,"MYLAYER",l->mWMSLayerName);
    boost::replace_all(JScommand,"MYURL",l->mUrl);
    boost::replace_all(JScommand,"MYATTRIBUTION",l->mWMSattribution);
    boost::replace_all(JScommand,"MYCODE",l->Code());
    doJavaScript(JScommand);
}

void formVielleCoupeRase::displayCommune(){
    if (commune_->currentIndex()!=0){
        std::string nameFile=mDico->mCadastre->createPolygonCommune(aMLabelCom.at(commune_->currentIndex()));
        boost::filesystem::path p(nameFile);
        OGREnvelope env= computeGlobalGeom(nameFile);

        std::string JScommand=std::string("station = new ol.layer.Vector({")+
                "source: new ol.source.Vector({"+
                "format: new ol.format.GeoJSON(),"+
                "url: 'tmp/"+p.filename().c_str()+"'}),"+
                "style:new ol.style.Style({"+
                "stroke: new ol.style.Stroke({"+
                "color: 'blue',"+
                "width: 2})"+
                "  }),"+
                "extent: ["+std::to_string(env.MinX) + ","+std::to_string(env.MinY)+ ","+std::to_string(env.MaxX)+ ","+std::to_string(env.MaxY)+"],"+
                "});"+
                "updateGroupeLayers();"+
                "map.getView().fit(station.getExtent());" ;
        doJavaScript(JScommand);
    }
}

OGREnvelope formVielleCoupeRase::computeGlobalGeom(std::string aFile){
    //std::cout << "computeGlobalGeom " << std::endl;
    OGREnvelope env= OGREnvelope();
    env.MinX =42247;
    env.MaxX = 295176;
    env.MinY = 21148;
    env.MaxY = 167719;

    const char *inputPath=aFile.c_str();
    GDALDataset * DS =  (GDALDataset*) GDALOpenEx( inputPath, GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DS != NULL )
    {
        OGRLayer * lay = DS->GetLayer(0);
        // union de tout les polygones du shp
        OGRFeature *poFeature;
        OGRGeometry * poGeom;
        OGRGeometry * poGeom2;
        std::unique_ptr<OGRMultiPolygon> multi = std::make_unique<OGRMultiPolygon>();
        OGRErr err;
        OGRMultiPolygon *poGeomM;

        int nbValidPol(0);
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            poFeature->GetGeometryRef()->flattenTo2D();
            switch (poFeature->GetGeometryRef()->getGeometryType()){
            case (wkbPolygon):
            {
                poGeom=poFeature->GetGeometryRef();
                poGeom->closeRings();
                poGeom = poGeom->Buffer(0.0);
                //poGeom->Simplify(1.0);
                err = multi->addGeometry(poGeom);
                if (err==OGRERR_NONE) nbValidPol++;
                break;
            }

            case wkbMultiPolygon:
            {
                poGeomM= poFeature->GetGeometryRef()->toMultiPolygon();
                int n(poGeomM->getNumGeometries());
                for (int i(0);i<n;i++){
                    poGeom=poGeomM->getGeometryRef(i);
                    poGeom->closeRings();
                    poGeom = poGeom->Buffer(0.0);
                    err = multi->addGeometry(poGeom);
                    if (err==OGRERR_NONE) nbValidPol++;
                }
                break;
            }
            default:
                std::cout << "Geometrie " << poFeature->GetFID() << ", type de geometrie non pris en charge ; " << poFeature->GetGeometryRef()->getGeometryName() << ", id " << poFeature->GetGeometryRef()->getGeometryType()<< std::endl;

                break;
            }
            if (err!=OGRERR_NONE){
                std::cout << "problem avec ajout de la geometrie " << poFeature->GetFID() << ", erreur : " << err <<  std::endl;
            }
            OGRFeature::DestroyFeature(poFeature);
        }
        if (nbValidPol>0){poGeom2 = multi->UnionCascaded();
            poGeom2 =poGeom2->Buffer(1.0);
            OGRGeometry * geomGlob = poGeom2->Simplify(1.0);
            geomGlob->getEnvelope(&env);
        }
    }else {
        std::cout << "computeGlobalGeom : je n'arrive pas à ouvrir " << aFile<< std::endl;
    }
    GDALClose(DS);
    return env;
}

void formVielleCoupeRase::validDraw(std::string geojson){

    std::string name0 = std::tmpnam(nullptr);
    std::string name1 = name0.substr(5,name0.size()-5);
    std::string aOut = mDico->File("TMPDIR")+"/"+name1+".geojson";
    std::ofstream ofs (aOut, std::ofstream::out);
    ofs << geojson;
    ofs.close();
    // lecture avec gdal
    GDALDataset * DS =  (GDALDataset*) GDALOpenEx( aOut.c_str(), GDAL_OF_VECTOR | GDAL_OF_READONLY, NULL, NULL, NULL );
    if( DS != NULL )
    {
        OGRLayer * lay = DS->GetLayer(0);

        OGRFeature *poFeature;
        while( (poFeature = lay->GetNextFeature()) != NULL )
        {
            surf=OGR_G_Area(poFeature->GetGeometryRef())/10000.0;
            //std::cout << "surface " << surf<< std::endl;
            polygValid=1;
            polyg=poFeature->GetGeometryRef()->exportToJson();
            geom = poFeature->GetGeometryRef();
        }
        doJavaScript("map.removeInteraction(draw);");
    }else {
        std::cout << "validDraw : je n'arrive pas à ouvrir " << aOut<< std::endl;
    }
    GDALClose(DS);
}

void formVielleCoupeRase::sendSummaryMail(){
    Wt::Mail::Message mail =Wt::Mail::Message();
    //mail.addHeader();
    mail.setFrom(Wt::Mail::Mailbox("JO.Lisein@uliege.be", "Lisein Jonathan"));
    mail.setBody(
                "\nVos données ont été encodée.\n"+
                Wt::WString::tr("mail.contact").toUTF8()+"\n--------------------------\n"+
                vosrefEdit_->valueText().toUTF8()+"\n"+
                nomEncoderEdit_->valueText().toUTF8()+"\n"+
                prenomEncoderEdit_->valueText().toUTF8()+"\n"+
                contactEncoderEdit_->valueText().toUTF8()+"\n"+
                contactEncoderGSMEdit_->valueText().toUTF8()+"\n"+
                typeContactEdit_->valueText().toUTF8()+"\n\n"+
                Wt::WString::tr("mail.acr").toUTF8()+"\n\n"+
                Wt::WString::tr("anneeVCR").toUTF8()+"\n"+
                anneeVCREdit_->currentText().toUTF8()+"\n"+
                Wt::WString::tr("regeNat").toUTF8()+"\n"+
                regeNatEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("vegeBloquante").toUTF8()+"\n"+
                vegeBloquanteEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("objectif").toUTF8()+"\n"+
                objectifEdit_->currentText().toUTF8()+"\n"+
                Wt::WString::tr("sp").toUTF8()+"\n"+
                spEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("sanit").toUTF8()+"\n"+
                sanitEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("trav.sylvi").toUTF8()+"\n"+
                travSylviEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("plantation").toUTF8()+"\n"+
                plantationEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("gibier").toUTF8()+"\n"+
                gibierEdit_->valueText().toUTF8()+"\n"+
                Wt::WString::tr("descriptionVCR").toUTF8()+"\n"+
                VCRdescriptionEdit_->valueText().toUTF8()+"\n"+
                "Surface de la coupe rase (ha) : "+
                Wt::WString(std::to_string(surf)).toUTF8()+"\n"
                );
    mail.setSubject(Wt::WString::tr("mail.titre").toUTF8());
    mail.addRecipient(Wt::Mail::RecipientType::To,Mail::Mailbox(contactEncoderEdit_->valueText().toUTF8(),nomEncoderEdit_->valueText().toUTF8()) );

    // création de la figure de localisation
    staticMap sm(mDico->getLayerBase("IGN"),geom);
    std::ifstream in;
    in.open(sm.getFileName(), std::ios::in);
    mail.addAttachment("image/png","localisationCoupe.png",&in);
    Mail::Client client;
    client.connect();
    client.send(mail);
    in.close();// après l'envoi!! car le addAttachement pointe ver le ifstream!
    mail =Wt::Mail::Message();
    mail.addRecipient(Wt::Mail::RecipientType::To,Mail::Mailbox("liseinjon@hotmail.com","Lisein Jonathan"));
    mail.setFrom(Wt::Mail::Mailbox("JO.Lisein@uliege.be", "Lisein Jonathan"));
    mail.setSubject("ACR - encodage");
    mail.setBody(SQLstring);
    client.send(mail);
}



ACRAnalytics::ACRAnalytics(const Wt::WEnvironment& env, std::string aFileDB) : Wt::WApplication(env),
    session()
{
    messageResourceBundle().use(docRoot() + "/encodageVCR");
    std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    setTheme(theme);
    // tout les style de wt gallery
    //useStyleSheet("resources/themes/bootstrap/5/bootstrap.bundle.min.css");
    //useStyleSheet("resources/themes/bootstrap/5/main.css");

    // CSS custom pour faire beau
    useStyleSheet("style/style.css");

    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");
    session.setConnection(std::move(sqlite3));
    session.mapClass<acr>("acr");
    typedef dbo::collection< dbo::ptr<acr> > obs;

    dbo::Transaction transaction{session};
    setTitle("Encodage anciennes coupes rases");
    root()->setMargin(0);
    root()->setPadding(0);

    Wt::WContainerWidget * content = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    content->setOverflow(Wt::Overflow::Scroll);
    content->addNew<Wt::WText>(WText::tr("coupe.rase.view"));
    Wt::WTable* table = content->addWidget(std::make_unique<Wt::WTable>());
    table->setHeaderCount(1);
    table->setWidth(Wt::WLength("100%"));
    //table->toggleStyleClass("table-striped",true);

    table->elementAt(0, 0)->addNew<Wt::WText>("id");
    table->elementAt(0, 1)->addNew<Wt::WText>("Date");
    table->elementAt(0, 2)->addNew<Wt::WText>("vosRef");
    table->elementAt(0, 3)->addNew<Wt::WText>("nom");
    table->elementAt(0, 4)->addNew<Wt::WText>("prenom");
    table->elementAt(0, 5)->addNew<Wt::WText>("contact");
    //table->elementAt(0, 5)->addNew<Wt::WText>("gsm");
    table->elementAt(0, 6)->addNew<Wt::WText>("typeContact");
    table->elementAt(0, 7)->addNew<Wt::WText>("anneeCoupe");
    table->elementAt(0, 8)->addNew<Wt::WText>("regeNat");
    table->elementAt(0, 9)->addNew<Wt::WText>("vegeBloquante");
    table->elementAt(0, 10)->addNew<Wt::WText>("objectif");
    table->elementAt(0, 11)->addNew<Wt::WText>("spCoupe");
    table->elementAt(0,12)->addNew<Wt::WText>("sanitCoupe");
    table->elementAt(0, 13)->addNew<Wt::WText>("travaux");
    table->elementAt(0, 14)->addNew<Wt::WText>("plantation");
    table->elementAt(0, 15)->addNew<Wt::WText>("gibier");
    table->elementAt(0, 16)->addNew<Wt::WText>("descr");
    table->elementAt(0, 17)->addNew<Wt::WText>("surf");

    obs rec = session.find<acr>();//.orderBy("datum DESC").limit(100);
    int i=1;
    for (const dbo::ptr<acr> &log : rec){
        // ici si un logs dans la db a un identifiant NULL (si on a oublié de coché "auto-increment" pour colonne id - on se retrouve avec un déréférencement.
        if (log.get()!=nullptr){
            table->elementAt(i,0)->addWidget(std::make_unique<Wt::WText>(std::to_string(log->id)));
            table->elementAt(i,1)->addWidget(std::make_unique<Wt::WText>(log->date));
            table->elementAt(i,2)->addWidget(std::make_unique<Wt::WText>(log->vosRef));
            table->elementAt(i,3)->addWidget(std::make_unique<Wt::WText>(log->nom));
            table->elementAt(i,4)->addWidget(std::make_unique<Wt::WText>(log->prenom));
            table->elementAt(i,5)->addWidget(std::make_unique<Wt::WText>(log->contact));
            table->elementAt(i,6)->addWidget(std::make_unique<Wt::WText>(log->typeContact));
            table->elementAt(i,7)->addWidget(std::make_unique<Wt::WText>(log->anneeCoupe));
            table->elementAt(i,8)->addWidget(std::make_unique<Wt::WText>(log->regeNat));
            table->elementAt(i,9)->addWidget(std::make_unique<Wt::WText>(log->vegeBloquante));
            table->elementAt(i,10)->addWidget(std::make_unique<Wt::WText>(log->objectif));
            table->elementAt(i,11)->addWidget(std::make_unique<Wt::WText>(log->spCoupe));
            table->elementAt(i,12)->addWidget(std::make_unique<Wt::WText>(log->sanitCoupe));
            table->elementAt(i,13)->addWidget(std::make_unique<Wt::WText>(log->travaux));
            table->elementAt(i,14)->addWidget(std::make_unique<Wt::WText>(log->plantation));
            table->elementAt(i,15)->addWidget(std::make_unique<Wt::WText>(log->gibier));
            table->elementAt(i,16)->addWidget(std::make_unique<Wt::WText>(log->descr));
            table->elementAt(i,17)->addWidget(std::make_unique<Wt::WText>(log->surf));
            i++;
        }
    }
}
