#include "textfill.h"
#include "www.h"

#ifdef INIT_ZIP_HEADER
	INIT_ZIP_HEADER
#endif

String fillText(const String & text, const List<String> & args) {
	String res = "";
	{
		int size = 0;
		for (const String & t : args)
			size += t.size();
		res.reserve(text.size() + size + 10);
	}
	auto it = args.begin();
	for (unsigned int i = 0; i < text.size() ; i++) {
		if (it != args.end() && text[i] == '{' && (i < (text.size() - 1)) && text[i+1] == '}') {
			res += *it;
			it++;
			i++;
			continue;
		}
		res += text[i];
	}
	return res;
}

String findText(const String & text) {
	return findCode(text);
}

char * findResourc(const std::string & file, unsigned long long & size) {
	return findResource(file, size);
}

String findFillText(const String & text, const List<String> & args) {
	return fillText(findCode(text), args);
}
