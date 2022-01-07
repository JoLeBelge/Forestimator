#include "analytics.h"

extern bool globTest;

Analytics::Analytics(std::string aFileDB) : session()
{
    //auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(sqliteDb);
    //auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>("/data1/Forestimator/data/analytics.db");
    auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");

    session.setConnection(std::move(sqlite3));
    session.mapClass<Log>("log");

    try {
        session.createTables();
        std::cout << "Created analytics database." << std::endl;
    } catch (std::exception& e) {
        std::cout << "Using existing analytics database...";
    }

}


void Analytics::addLog(const Wt::WEnvironment &env, int user_id, std::string page, int cat){

    if(env.agentIsSpiderBot())return;

    dbo::Transaction transaction{session};

    std::unique_ptr<Log> log{new Log()};
    log->datum = time(0);
    log->date = Wt::WDate::currentDate();
    log->ip = env.clientAddress();
    log->client = env.userAgent();
    log->ipath = page;
    log->id_user = user_id;
    log->categorie = cat;

    //std::cout << log->datum << log->client << log->ipath << std::endl;

    dbo::ptr<Log> logPtr = session.add(std::move(log));

}

bool Analytics::logExist(const Wt::WEnvironment &env, std::string page, typeLog cat){
    bool aRes(1);
    dbo::Transaction transaction{session};

    /*dbo::ptr<Log> logPtr = session.find<Log>().where("date = ?").bind(Wt::WDate::currentDate().toString("yyyy-MM-dd")).where("ipath = ?").bind(page).where("ip = ?").bind(env.clientAddress());
        if (logPtr.get()==nullptr){aRes=0;} else{
         if (globTest){std::cout << " le log existe déjà pour cet utilisateur !" << std::endl;}
        }
        */

    int c=session.query<int>("select count(1) from Log").where("date = ?").bind(Wt::WDate::currentDate().toString("yyyy-MM-dd")).where("ipath = ?").bind(page).where("ip = ?").bind(env.clientAddress()).where("cat = ?").bind((int (cat)));
    if (c==0){aRes=0;} else{
        if (globTest){std::cout << " le log existe déjà pour cet utilisateur !" << std::endl;}
    }
    return aRes;
}


PageAnalytics::PageAnalytics(const Wt::WEnvironment& env, std::string aFileDB) : Wt::WApplication(env),
    session()
{
    auto sqlite3 = Wt::cpp14::make_unique<dbo::backend::Sqlite3>(aFileDB);
    sqlite3->setProperty("show-queries", "false");
    session.setConnection(std::move(sqlite3));
    session.mapClass<Log>("log");

    setTitle("Forestimator - Stats");

    auto layout = root()->setLayout(Wt::cpp14::make_unique<Wt::WVBoxLayout>());
    root()->setMargin(0);
    root()->setPadding(0);

    Wt::WContainerWidget * content = layout->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    content->setOverflow(Wt::Overflow::Scroll);
    content->addNew<Wt::WText>("Dernieres stats brutes :");

    auto table = content->addWidget(Wt::cpp14::make_unique<Wt::WTable>());
    table->setHeaderCount(1);
    table->setWidth(Wt::WLength("100%"));

    table->elementAt(0, 0)->addNew<Wt::WText>("Date");
    table->elementAt(0, 1)->addNew<Wt::WText>("IP");
    table->elementAt(0, 2)->addNew<Wt::WText>("Client");
    table->elementAt(0, 3)->addNew<Wt::WText>("Page");
    table->elementAt(0, 4)->addNew<Wt::WText>("Categorie");

    if (globTest){std::cout << " statistiques brutes : début de transaction avec la BD analytics " << std::endl;}
    dbo::Transaction transaction{session};
    if (globTest){std::cout << " done " << std::endl;}

    typedef dbo::collection< dbo::ptr<Log> > Logs;
    Logs logs = session.find<Log>().orderBy("datum DESC").limit(100);

    int i=1;
    for (const dbo::ptr<Log> &log : logs){
        if (globTest){std::cout << " log " << log->datum << " : " << log->ip << " : " << log->ipath << std::endl;}

        time_t now=log->datum;
        //tm *ltm = localtime(&now);
        table->elementAt(i,0)->addWidget(Wt::cpp14::make_unique<Wt::WText>(ctime(&now)));
        table->elementAt(i,1)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ip));
        table->elementAt(i,2)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->client));
        table->elementAt(i,3)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ipath));
        table->elementAt(i,4)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->getCat()));

        i++;
    }

}
