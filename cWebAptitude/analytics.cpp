#include "analytics.h"






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

void Analytics::addLog(const Wt::WEnvironment &env, int user_id, std::string page){

    dbo::Transaction transaction{session};

    std::unique_ptr<Log> log{new Log()};
    log->datum = time(0);
    log->ip = env.clientAddress();
    log->client = env.userAgent();
    log->ipath = page;
    log->id_user = user_id;

    //std::cout << log->datum << log->client << log->ipath << std::endl;

    dbo::ptr<Log> logPtr = session.add(std::move(log));

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

    auto content = layout->addWidget(Wt::cpp14::make_unique<Wt::WContainerWidget>());
    content->addNew<Wt::WText>("Dernieres stats brutes :");
    auto table = content->addWidget(Wt::cpp14::make_unique<Wt::WTable>());
    table->setHeaderCount(1);
    table->setWidth(Wt::WLength("100%"));

    table->elementAt(0, 0)->addNew<Wt::WText>("Date");
    table->elementAt(0, 1)->addNew<Wt::WText>("IP");
    table->elementAt(0, 2)->addNew<Wt::WText>("Client");
    table->elementAt(0, 3)->addNew<Wt::WText>("Page");

    dbo::Transaction transaction{session};

    typedef dbo::collection< dbo::ptr<Log> > Logs;
    Logs logs = session.find<Log>().orderBy("datum DESC").limit(100);

    int i=1;
    for (const dbo::ptr<Log> &log : logs){
        std::cout << " log " << log->datum << " : " << log->ip << " : " << log->ipath << std::endl;

        time_t now=log->datum;
        //tm *ltm = localtime(&now);

        table->elementAt(i,0)->addWidget(Wt::cpp14::make_unique<Wt::WText>(ctime(&now)));
        table->elementAt(i,1)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ip));
        table->elementAt(i,2)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->client));
        table->elementAt(i,3)->addWidget(Wt::cpp14::make_unique<Wt::WText>(log->ipath));

        i++;
    }
    //content->

}