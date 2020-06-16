#include "cwebaptitude.h"


//const char *cl[] = { "FEE", "CS" };
//std::vector<std::string> classes = {"Fichier Ecologique des Essences", "Catalogue des Stations"};
//extern std::vector<std::string> classes;

cWebAptitude::cWebAptitude(WApplication* app)
    : WContainerWidget()
{
    m_app = app;
    std::string aBD=loadBDpath();
    mDico=new cDicoApt(aBD);

    //setOverflow(Overflow::Auto);
    //setPadding(20);
    setContentAlignment(AlignmentFlag::Center | AlignmentFlag::Middle);
	setStyleClass("carto_div");
	WVBoxLayout * layoutGlobal = setLayout(cpp14::make_unique<WVBoxLayout>());
	
	
    /*	NAVIGATION BAR	*/
	auto navigation = layoutGlobal->addWidget(cpp14::make_unique<WNavigationBar>());
	navigation->setTitle("   <strong>Forestimator</strong>   ", "/");
	navigation->setResponsive(true);
	navigation->addStyleClass("carto_menu");
	// Setup a Left-aligned menu. Remplace le menu de droite.
	auto left_menu = cpp14::make_unique<WMenu>();
	auto left_menu_ = navigation->addMenu(std::move(left_menu));
	left_menu_->addItem("Présentation")
		->setLink(WLink(LinkType::InternalPath, "/presentation"));
	auto menuitem_carto = left_menu_->addItem("Cartographie");
	menuitem_carto->setLink(WLink(LinkType::InternalPath, "/cartographie"));
	auto menuitem_analyse = left_menu_->addItem("Analyse");
	menuitem_analyse->setLink(WLink(LinkType::InternalPath, "/analyse"));
		
		
    /* 
     * création d'un stack pour les différentes pages du sites
     * page 1 ; autécologie
     * page 2 ; statistique parcellaire
     * si this n'as pas de layout mais que page_carto a un layout, le rendu est très différent que si this a un layout.	
     */
    WStackedWidget * topStack  = layoutGlobal->addWidget(cpp14::make_unique<WStackedWidget>());
    // page principale
    WContainerWidget * page_carto = topStack->addNew<WContainerWidget>();
    // page de statistique
    WContainerWidget * page2 = topStack->addNew<WContainerWidget>(); // pas encore utilisée
    // TODO remove page2 et modifier Parcellaire qui l'utilise

    /* MAP div */
    auto layout_carto = page_carto->setLayout(cpp14::make_unique<WHBoxLayout>());
    page_carto->setHeight("100%"); // oui ça ca marche bien! reste plus qu'à empêcher la carte de s'escamoter.
    page_carto->setOverflow(Overflow::Visible); // non pas d'overflow pour la carte, qui est dans page_carto

    /* Partie droite couches-légende-analyse */
    auto content_info = cpp14::make_unique<WContainerWidget>();
    WContainerWidget * content_info_ = content_info.get();
    WVBoxLayout * layout_info = content_info_->setLayout(cpp14::make_unique<WVBoxLayout>());
    content_info_->setOverflow(Overflow::Auto);
    content_info_->setWidth("400px"); // TODO CSS resize @min-width
    content_info_->setMinimumSize(400,0);


    /* 	SOUS-MENU droite    */
    // Create a stack where the contents will be located.
	stack_info = layout_info->addWidget(cpp14::make_unique<WStackedWidget>());
	stack_info->setOverflow(Overflow::Auto);
    std::unique_ptr<WMenu> menu_ = cpp14::make_unique<WMenu>();
    WMenu * right_menu = navigation->addMenu(std::move(menu_), Wt::AlignmentFlag::Right);
    auto menuitem2_cartes = right_menu->addItem("Couches");
	auto menuitem2_legend = right_menu->addItem("Légende");
    menuitem2_analyse = right_menu->addItem("Analyse");
    menuitem2_cartes->clicked().connect([=] {stack_info->setCurrentIndex(2);});
	menuitem2_legend->clicked().connect([=] {stack_info->setCurrentIndex(0);});
    menuitem2_analyse->clicked().connect([=] {stack_info->setCurrentIndex(1);});    
    
    auto content_legend = stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    auto content_analyse = stack_info->addWidget(cpp14::make_unique<WContainerWidget>());


	/*	MAPS	*/
    printf("create map\n");
    auto map = cpp14::make_unique<WOpenLayers>(mDico);
    mMap = map.get();
    mMap->setWidth("70%");
    mMap->setMinimumSize(400,0);
    mMap->setOverflow(Overflow::Visible); 
    color col= mDico->Apt2col(2);
    WCssDecorationStyle T;
    T.setForegroundColor(WColor("black"));
    //T.setBackgroundImage("data/img/T.png");
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".T", T);
    col=mDico->Apt2col(1);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".O", T);
    col=mDico->Apt2col(3);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".TE", T);
    col=mDico->Apt2col(4);
    T.setBackgroundColor(WColor(col.mR,col.mG,col.mB));
    app->styleSheet().addRule(".E", T);
    // à nouveau, ça provoque un dangling ptr si je passe par un unique_ptr, au lieu de ça je fait un conteneur unique_ptr et le groupLayer est créé avec un new et rempli le conteneur parent
    // comme pour le bug dans parcellaire
    //auto groupL = cpp14::make_unique<groupLayers>(mDico,this,content_legend.get(),mMap,m_app);
    auto content_GL = stack_info->addWidget(cpp14::make_unique<WContainerWidget>());
    content_GL->setStyleClass("content_GL");


    /* CHARGE ONGLET COUCHES & LEGENDE */
    printf("create GL\n");
    mGroupL = new groupLayers(mDico,content_GL,content_legend,mMap,m_app);


    /* CHARGE ONGLET ANALYSES */
    printf("create PA\n");
    mPA = new parcellaire(content_analyse,mGroupL,m_app,topStack,page2);
    std::cout << "PA done" << std::endl;


    // first route TODO check si necessaire
    printf("route : %s\n",m_app->internalPath().c_str());
    if (m_app->internalPath() == "/analyse"){
		menuitem_analyse->addStyleClass("active");
		menuitem2_analyse->addStyleClass("active");
		//menuitem2_analyse->toggleStyleClass("notvisible",true,);
		menuitem2_analyse->setHidden(false);
		stack_info->setCurrentIndex(1);
		mMap->setWidth("60%");
		// TODO css min-size [menu_analyse] display:none if width<900
	}else{
		menuitem_carto->addStyleClass("active");
		stack_info->setCurrentIndex(2);
		menuitem2_cartes->addStyleClass("active");
		//menuitem2_analyse->toggleStyleClass("notvisible",true);
		menuitem2_analyse->setHidden(true);
		// TODO css @media-width<1200 -> map 60%  @media-width<900 -> [div stack] display:blocks et overflow:auto [map] width:90%  [linfo] min-height: 600px;
	}


    // maintenant que tout les objets sont crées, je ferme la connection avec la BD sqlite3, plus propre
    mDico->closeConnection();


	/*	ACTIONS		*/
    mMap->doubleClicked().connect(mMap->slot);
    // reviens sur l'onglet légende si on est sur l'onglet parcellaire
    mMap->doubleClicked().connect([=]{menuitem2_legend->select();}); //itLegend
    // et dans wt_config, mettre à 500 milliseconde au lieu de 200 pour le double click
    mMap->xy().connect(std::bind(&groupLayers::extractInfo,mGroupL, std::placeholders::_1,std::placeholders::_2));

    mMap->clicked().connect(std::bind(&WOpenLayers::filterMouseEvent,mMap,std::placeholders::_1));
    mMap->polygId().connect(std::bind(&parcellaire::computeStatAndVisuSelectedPol,mPA, std::placeholders::_1));
    
    // je divise la fenetre en 2 dans la largeur pour mettre la carte à gauche et à droite une fenetre avec les infos des couches
    printf("Move divs\n");
    layout_carto->addWidget(std::move(map), 0);
    layout_carto->addWidget(std::move(content_info), 1);
    printf("divs moved -> lets show HTML\n");

}
