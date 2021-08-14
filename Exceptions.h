#pragma once

class qe3_exception : public std::exception {
public:
	qe3_exception(const char* what) : exception(what) {};
	~qe3_exception();
};

class qe3_cmd_exception : public qe3_exception {
public:
	qe3_cmd_exception(const char* what) : qe3_exception(what) {};
};

inline void Error(const char* error) { throw qe3_exception(error); }

const char* SEHExceptionString(DWORD code);
int		CrashSave(const char* reason);
void	Terminate(const char* badnews);

