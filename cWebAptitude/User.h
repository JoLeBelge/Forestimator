#include <Wt/Dbo/Types.h>
#include <Wt/WGlobal.h>

namespace dbo = Wt::Dbo;

class User;
using AuthInfo = Wt::Auth::Dbo::AuthInfo<User>;

class User {
public:
  template<class Action>
  void persist(Action& a)
  {
  }
};

DBO_EXTERN_TEMPLATES(User)
