#ifndef _FORESTIMATOR_TOOLS_
#define _FORESTIMATOR_TOOLS_
#pragma once

#include "Wt/Mail/Message.h"
#include "Wt/Mail/Client.h"
#include <iostream>
#include <bits/stdc++.h>
#include "cdicoapt.h"

using namespace std;

namespace tools {
    Wt::Mail::Message createMail(
        const std::string &from,
        const std::string &fromName,
        const std::string &to,
        const std::string &subject,
        const std::string &body
    );
    bool sendMail(const Wt::Mail::Message &mail, bool useForestimatorDomain = false);
}
#endif