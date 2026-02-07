#include "Parser.hpp"

// Basic IRC line parser.
IrcMessage parseIrcLine(const std::string& raw) {
	IrcMessage m;
	m.hasTrailing = false;
	const std::string& s = raw;
	size_t i = 0, n = s.size();

	if (i < n && s[i] == ':') {
		size_t sp = s.find(' ', i);
		if (sp != std::string::npos) { m.prefix = s.substr(1, sp-1); i = sp+1; }
		else { m.prefix = s.substr(1); return m; }
	}

	size_t sp = s.find(' ', i);
	if (sp == std::string::npos) { m.command = s.substr(i); return m; }
	m.command = s.substr(i, sp - i); i = sp + 1;

	while (i < n) {
		while (i < n && s[i] == ' ') ++i;
		if (i >= n) break;
		if (s[i] == ':')
		{
			m.hasTrailing = true;
			m.trailing = s.substr(i+1);
			break;
		}
		size_t next = s.find(' ', i);
		if (next == std::string::npos) { m.params.push_back(s.substr(i)); break; }
		m.params.push_back(s.substr(i, next - i));
		i = next + 1;
		while (i<n && s[i]==' ') ++i;
	}
	return m;
}
