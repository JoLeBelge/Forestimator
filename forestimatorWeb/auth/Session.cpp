#include "Session.h"

#include "Wt/Auth/AuthService.h"
#include "Wt/Auth/HashFunction.h"
#include "Wt/Auth/PasswordService.h"
#include "Wt/Auth/PasswordStrengthValidator.h"
#include "Wt/Auth/PasswordVerifier.h"
#include "Wt/Auth/Dbo/AuthInfo.h"
#include "Wt/Dbo/backend/Sqlite3.h"

using namespace Wt;

extern bool globTest;

namespace {

Auth::AuthService myAuthService;
Auth::PasswordService myPasswordService(myAuthService);
//std::vector<std::unique_ptr<Auth::OAuthService>> myOAuthServices;

}

void Session::configureAuth()
{
    myAuthService.setAuthTokensEnabled(true, "logincookie");
    myAuthService.setEmailVerificationEnabled(true);
    //myAuthService.setEmailVerificationRequired(true);

    std::unique_ptr<Auth::PasswordVerifier> verifier
            = std::make_unique<Auth::PasswordVerifier>();
    verifier->addHashFunction(std::make_unique<Auth::BCryptHashFunction>(7));
    myPasswordService.setVerifier(std::move(verifier));
    myPasswordService.setAttemptThrottlingEnabled(true);
    //myPasswordService.setStrengthValidator(std::make_unique<Auth::PasswordStrengthValidator>());

    // paramètrer le validateur de password
    std::unique_ptr<Auth::PasswordStrengthValidator> validator= std::make_unique<Auth::PasswordStrengthValidator>();
    //validator->setMinimumPassPhraseWords(Wt::Auth::PasswordStrengthValidator::Disabled);
    validator->setMinimumLength(Wt::Auth::PasswordStrengthType::OneCharClass, 4);
    validator->setMinimumLength(Wt::Auth::PasswordStrengthType::TwoCharClass, 4);
    validator->setMinimumLength(Wt::Auth::PasswordStrengthType::ThreeCharClass, 4);
    validator->setMinimumLength(Wt::Auth::PasswordStrengthType::FourCharClass, 4);

    myPasswordService.setStrengthValidator(std::move(validator));

    /*if (Auth::GoogleService::configured())
    myOAuthServices.push_back(std::make_unique<Auth::GoogleService>(myAuthService));

  if (Auth::FacebookService::configured())
    myOAuthServices.push_back(std::make_unique<Auth::FacebookService>(myAuthService));

  for (unsigned i = 0; i < myOAuthServices.size(); ++i)
    myOAuthServices[i]->generateRedirectEndpoint();
    */
}

Session::Session(const std::string& sqliteDb)
{
    if (globTest){std::cout << "new Session()" << std::endl;}
    auto connection = std::make_unique<Dbo::backend::Sqlite3>(sqliteDb);

    connection->setProperty("show-queries", "false");

    setConnection(std::move(connection));

    // la classe User c'est moi qui la défini, les infos de cette classe sont dans la table user
    mapClass<User>("user");
    mapClass<AuthInfo>("auth_info");
    mapClass<AuthInfo::AuthIdentityType>("auth_identity");
    mapClass<AuthInfo::AuthTokenType>("auth_token");

    try {
        createTables();
        std::cout << "Created database." << std::endl;
    } catch (std::exception& e) {
        //std::cerr << e.what() << std::endl;
        //std::cout << "Using existing database...";
    }

    users_ = std::make_unique<UserDatabase>(*this);

}

Auth::AbstractUserDatabase& Session::users()
{
    return *users_;
}

dbo::ptr<User> Session::user() const
{
    std::cout << "get user()..." << std::endl;
    if (login_.loggedIn()) {
        dbo::ptr<AuthInfo> authInfo = users_->find(login_.user());
        std::cout << "connected\n" << std::endl;
        return authInfo->user();
    } else{
        std::cout << "not connected\n" << std::endl;
        return dbo::ptr<User>();
    }
}

const Auth::AuthService& Session::auth()
{
    return myAuthService;
}

const Auth::AbstractPasswordService& Session::passwordAuth()
{
    return myPasswordService;
}

const std::vector<const Auth::OAuthService *> Session::oAuth()
{
    std::vector<const Auth::OAuthService *> result;
    /*for (auto &auth : myOAuthServices) {
    result.push_back(auth.get());
  }*/
    return result;
}
