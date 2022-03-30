#pragma once
#include <DPLib.conf.h>

using __DP_LIB_NAMESPACE__::String;
using __DP_LIB_NAMESPACE__::List;

String fillText(const String & text, const List<String> & args);
String findText(const String & text);
char * findResourc(const std::string & file, unsigned long long & size);
String findFillText(const String & text, const List<String> & args);
