#include "ConsoleLoop.h"

void calcSizeAndText(double origin, double & res, String & res_s) {
	res_s = "b";
	static Map<String, String> suff {
		{ "b", "kb" },
		{ "kb", "mb" },
		{ "mb", "gb" },
		{ "gb", "tb" },
		{ "tb", "pb" }
	};
	while (origin > 1024.0) {
		res_s = suff[res_s];
		origin = origin / 1024.0;
	}
	res = origin;
}

void calcToBytes(double origin, String res_s, double & res) {
	static Map<String, String> suff_s {
		{ "kb/s", "b/s" },
		{ "mb/s", "kb/s" },
		{ "gb/s", "mb/s" },
		{ "tb/s", "gb/s" },
		{ "pb/s", "tb/s" }
	};
	static Map<String, String> suff_ms {
		{ "kb/ms", "b/ms" },
		{ "mb/ms", "kb/ms" },
		{ "gb/ms", "mb/ms" },
		{ "tb/ms", "gb/ms" },
		{ "pb/ms", "tb/ms" }
	};
	if (suff_s.find(res_s) != suff_s.end()) {
		while (res_s != "b/s") {
			res_s = suff_s[res_s];
			origin = origin * 1024.0;
		}
	}
	if (suff_ms.find(res_s) != suff_ms.end()) {
		while (res_s != "b/ms") {
			res_s = suff_ms[res_s];
			origin = origin * 1024.0;
		}
		origin = origin/1000;
	}
	res = origin;
}
