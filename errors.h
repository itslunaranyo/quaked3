//==============================
//	errors.h
//==============================

#ifndef __ERRORS_H__
#define __ERRORS_H__

extern bool g_bWarningOrError;

class qe3_exception : public std::exception { 
public: 
	qe3_exception(const char* what) : exception(what) {}; 
};

class qe3_cmd_exception : public qe3_exception { 
public: 
	qe3_cmd_exception(const char* what) : qe3_exception(what) {}; 
};

void	Warning(char *warning, ...);
void	Error(char *error, ...);
void	ReportError(qe3_exception& ex);


#endif // __ERRORS_H__