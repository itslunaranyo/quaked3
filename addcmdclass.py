import sys, os

headerTemplate = """//==============================
//	{1}.h
//==============================

#ifndef {0}
#define {0}

class {1} : public Command
{{
public:
	{1}();
	~{1}();

private:
	void Do_Impl();
	void Undo_Impl();
	void Redo_Impl();
	void Select_Impl();

}};

#endif	// {0}
"""

bodyTemplate = """//==============================
//	{1}.cpp
//==============================

#include "qe3.h"

{1}::{1}()
{{
	// state = LIVE;
}}

{1}::~{1}()
{{
	
}}

//==============================

void {1}::Do_Impl()
{{
	
}}

void {1}::Undo_Impl()
{{
	
}}

void {1}::Redo_Impl()
{{
	
}}

void {1}::Select_Impl()
{{
	
}}


"""

def AddAGoddamnClass(name):	
	if name[0:3] != "Cmd":
		ig = IncludeGuardString(name)
		name = "Cmd" + name
	else:
		ig = IncludeGuardString(name[3:])
		
	print("Creating base class files for " + name)
	
	header = headerTemplate.format(ig, name)
	body = bodyTemplate.format(ig, name)

	hname = name + ".h"
	cppname = name + ".cpp"
	
	if os.path.isfile(hname) or os.path.isfile(cppname):
		print("Files with that name already exist.")
		return
	
	doth = open(hname, 'w')
	doth.write(header)
	doth.close()

	dotcpp = open(cppname, 'w')
	dotcpp.write(body)
	dotcpp.close()

	
def IncludeGuardString(name):
	includeguard = "__COMMAND"
	for ch in name:
		if ch.isupper():
			includeguard += "_"
		includeguard += ch.upper()
	if includeguard[0] != "_":
		includeguard = "_" + includeguard
	includeguard += "_H__"
	return includeguard
	
if len(sys.argv) == 1:
	print("Specify a name, jackass")
	exit()

name = sys.argv[1]
AddAGoddamnClass(name)