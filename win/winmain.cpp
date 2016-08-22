#include <Windows.h>

extern "C" int main(int argc, char *argv[]);

extern "C" const char *getPackageDataDir()
{
	static char pathname[1024];

	if( pathname[0] == '\0' ) {
		int len = GetModuleFileNameA(NULL, pathname, sizeof(pathname));
		while( len > 0 && pathname[len-1] != '\\' )
			--len;
		pathname[len] = '\0';
	}
	return pathname;
}

extern "C" const char *getGladeFilePathname(const char *fname)
{
	static char pathname[1024];

	int len = GetModuleFileNameA(NULL, pathname, sizeof(pathname));
	while( len > 0 && pathname[len-1] != '\\' )
		--len;
	strcpy(pathname + len, "gtkbuilder\\");
	strcat(pathname + len, fname);
#ifdef _DEBUG
	// prepare path for exe located in win\Debug subdirectory
	if( GetFileAttributesA(pathname) == INVALID_FILE_ATTRIBUTES ) {
		strcpy(pathname + len, "..\\..\\src\\");
		strcat(pathname + len, fname);
	}
#endif
	return pathname;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     cmdLine,
                     int       nCmdShow)
{
	char *args[20] = { "reminderer" };
	bool inQuote = false, isEnd = false;
	char *argBeg = NULL, *argEnd;
	int argCount = 1;

	for(int i = 0; !isEnd && argCount < 20; ++i) {
		argEnd = NULL;
		switch( cmdLine[i] ) {
		case '"':
			if( inQuote ) {
				argEnd = cmdLine + i;
			}else if( argBeg == NULL ) {
				argBeg = cmdLine + i + 1;
				inQuote = true;
			}
			break;
		case ' ':
		case '\t':
		case '\n':
			if( !inQuote && argBeg != NULL )
				argEnd = cmdLine + i;
			break;
		case '\0':
			if( argBeg != NULL )
				argEnd = cmdLine + i;
			isEnd = true;
			break;
		default:
			if( argBeg == NULL )
				argBeg = cmdLine + i;
			break;
		}
		if( argEnd != NULL ) {
			args[argCount++] = argBeg;
			*argEnd = '\0';
			inQuote = false;
			argBeg = NULL;
		}
	}
	return main(argCount, args);
}
