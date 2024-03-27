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

    cont = tpl->bindWidget("contACR", std::make_unique<WContainerWidget>());
    cont->addNew<Wt::WText>(WString::tr("sectionEncoder"));

    WTable * table= cont->addNew<WTable>();
    int row=0;

    nomEncoderEdit_ = table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    table->elementAt(row,1)->setMinimumSize("300px", "300px");
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("nom")));
    label->setBuddy(nomEncoderEdit_);
    nomEncoderEdit_->setPlaceholderText(WString::tr("nom.ph"));
    nomEncoderEdit_->setMinimumSize("200px", "100%");
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
    ++row;
    keepInTouch= table->elementAt(row,1)->addWidget(std::make_unique<WCheckBox>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("kit")));
    ++row;
    ContactEdit_= table->elementAt(row,1)->addWidget(std::make_unique<WLineEdit>());
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("contactACR")));
    ContactEdit_->setPlaceholderText(WString::tr("contactACR.ph"));

    cont->addNew<Wt::WText>(WString::tr("sectionVCR"));
    table= cont->addNew<WTable>();
    row=0;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("anneeVCR")));
    ++row;
    anneeVCREdit_= table->elementAt(row,0)->addWidget(std::make_unique<WComboBox>());
     anneeVCREdit_->addItem("en quelle année a eu lieu la coupe?");
    for (int y(2019);y>2009;y--){
        anneeVCREdit_->addItem("coupé en "+std::to_string(y)+" (il y a "+std::to_string(2024-y)+" ans)");
    }
    anneeVCREdit_->addItem("coupé avant 2009 (plus de 15 ans)");
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("regeNat")));
    ++row;
    regeNatEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    regeNatEdit_->setMinimumSize("200px", "100%");
    regeNatEdit_->setPlaceholderText(WString::tr("regeNat.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("vegeBloquante")));
    ++row;
    vegeBloquanteEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WLineEdit>());
    vegeBloquanteEdit_->setMinimumSize("200px", "100%");
    vegeBloquanteEdit_->setPlaceholderText(WString::tr("vegeBloquante.ph"));
    ++row;
    label = table->elementAt(row,0)->addWidget(std::make_unique<WLabel>(WString::tr("descriptionVCR")));
    ++row;
    VCRdescriptionEdit_= table->elementAt(row,0)->addWidget(std::make_unique<WTextArea>());
    VCRdescriptionEdit_->setColumns(40);
    VCRdescriptionEdit_->setRows(5);
    VCRdescriptionEdit_->setPlaceholderText(WString::tr("descriptionVCR.ph"));

    nomEncoderEdit_->enterPressed().connect(prenomEncoderEdit_, &WWidget::setFocus);
    prenomEncoderEdit_->enterPressed().connect(contactEncoderEdit_, &WWidget::setFocus);
    contactEncoderEdit_->enterPressed().connect(ContactEdit_, &WWidget::setFocus);
    ContactEdit_->enterPressed().connect(anneeVCREdit_, &WWidget::setFocus);
    anneeVCREdit_->changed().connect(regeNatEdit_, &WWidget::setFocus);
    regeNatEdit_->enterPressed().connect(vegeBloquanteEdit_, &WWidget::setFocus);
    vegeBloquanteEdit_->enterPressed().connect(VCRdescriptionEdit_, &WWidget::setFocus);

    cont = tpl->bindWidget("contLoca", std::make_unique<WContainerWidget>());
    cont->addStyleClass("encodage");

    WVBoxLayout * la = cont->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    cont= la->addWidget(std::make_unique<WContainerWidget>());
    cont->addWidget(std::make_unique<Wt::WText>(WString::tr("titreLocalisation")));
    cont= la->addWidget(std::make_unique<WContainerWidget>());
    WHBoxLayout * layoutH = cont->setLayout(Wt::cpp14::make_unique<Wt::WHBoxLayout>());
    auto smart_map = std::make_unique<Wol>();
    Wol * map = smart_map.get();
    std::ifstream t("/home/jo/app/Forestimator/data/js/initOL_vcr.js");
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
    std::vector<std::string> vLs={"IGN", "ortho2020"};
    for (std::string code : vLs){
        std::shared_ptr<layerBase> aL1=mDico->getLayerBase(code);
        WText * wtext1= cont->addNew<Wt::WText>(aL1->Nom());
        wtext1->addStyleClass("encodageLayer");
        cont->addNew<WBreak>();
        wtext1->clicked().connect([=] {this->displayLayer(aL1->Code());});
    }
    cont->addNew<WBreak>();
    bCancel =cont->addNew<WPushButton>(WString::tr("initPol"));
    bCancel->setMinimumSize("100%"," ");
    bCancel->addStyleClass("btn btn-danger");
    bCancel->clicked().connect([=] {doJavaScript("acr_src.clear();map.addInteraction(draw);"); polygValid=0;});
    WPushButton * bUndo =cont->addNew<WPushButton>(WString::tr("cancelPoint"));
    bUndo->setMinimumSize("100%"," ");
    bUndo->clicked().connect([=] {doJavaScript("draw.removeLastPoint();");});
    map->polygGeojson().connect(std::bind(&formVielleCoupeRase::validDraw,this, std::placeholders::_1));

    WPushButton * bSubmit =  cont->addNew<WPushButton>(WString::tr("submit"));
    bSubmit->setMinimumSize("70%","100px");
    bSubmit->addStyleClass("btn btn-success");
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
    } else {
        // sauver la coupe rase dans la BD
        int rc;
        sqlite3 *db_;
        rc = sqlite3_open(mBDFile.c_str(), &db_);
        if( rc )  {} else {
                //std::cout << " sauve la coupe rase " << std::endl;
                sqlite3_stmt * stmt;
                std::string SQLstring="INSERT INTO acr (nom,prenom,contact,keepInTouch,contact2,anneeCoupe,regeNat,vegeBloquante,descr,surf,polygon) VALUES ('"
                        +nomEncoderEdit_->valueText().toUTF8()+"',"
                        +"'"+prenomEncoderEdit_->valueText().toUTF8()+"',"
                        +"'"+contactEncoderEdit_->valueText().toUTF8()+"',"
                        +std::to_string(keepInTouch->isChecked())+","
                        +"'"+ContactEdit_->valueText().toUTF8()+"',"
                        +"'"+anneeVCREdit_->currentText().toUTF8()+"',"
                        +"'"+regeNatEdit_->valueText().toUTF8()+"',"
                        +"'"+vegeBloquanteEdit_->valueText().toUTF8()+"',"
                        +"'"+VCRdescriptionEdit_->valueText().toUTF8()+"',"
                        +std::to_string(surf)+","
                        +"'"+polyg+"');";
                //std::cout << "sql : " << SQLstring << std::endl;
                sqlite3_prepare_v2(db_, SQLstring.c_str(), -1, &stmt, NULL );
                // applique l'update
                sqlite3_step( stmt );
                sqlite3_finalize(stmt);



            }
        sqlite3_close(db_);

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

void formVielleCoupeRase::vider(bool all){
    //std::cout << "vider formulaire"<< std::endl;
   if(all){
   nomEncoderEdit_->setText("");
   prenomEncoderEdit_->setText("");
   contactEncoderEdit_->setText("");
   keepInTouch->setChecked(0);
   ContactEdit_->setText("");
   }
   anneeVCREdit_->setCurrentIndex(0);
   regeNatEdit_->setText("");
   vegeBloquanteEdit_->setText("");
   VCRdescriptionEdit_->setText("");
   polygValid=0;
   polyg="";
   doJavaScript("acr_src.clear();map.addInteraction(draw);");
}

void formVielleCoupeRase::loadStyles(){
    //std::shared_ptr<Wt::WBootstrap5Theme> theme = std::make_shared<Wt::WBootstrap5Theme>();
    auto theme = std::make_shared<Wt::WBootstrapTheme>();
    theme->setVersion(Wt::BootstrapVersion::v3);
    theme->setResponsive(true);
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
    this->clicked().connect(this->slot);
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
    //std::cout << "valid draw \n" << geojson << std::endl;
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
        }
        doJavaScript("map.removeInteraction(draw);");
    }else {
        std::cout << "validDraw : je n'arrive pas à ouvrir " << aOut<< std::endl;
    }
    GDALClose(DS);
}



