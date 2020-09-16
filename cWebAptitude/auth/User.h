#include <Wt/Dbo/Types.h>
#include <Wt/WGlobal.h>

namespace dbo = Wt::Dbo;

class User;
using AuthInfo = Wt::Auth::Dbo::AuthInfo<User>;

class User {
public:

  /*dbo::collection<dbo::ptr<AuthInfo>> authInfos;
  bool expert;
  std::string name;
    */

  template<class Action>
  void persist(Action& a)
  {
      //dbo::field(a, expert, "ModeExpert");
      //dbo::hasMany(a, authInfos, dbo::ManyToOne, "user");
  }

};

DBO_EXTERN_TEMPLATES(User)
