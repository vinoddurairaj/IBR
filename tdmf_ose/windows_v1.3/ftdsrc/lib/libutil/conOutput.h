int consoleOutput_Launch( const char* szAppName,        //[in] : file name app to be launched
                          const char* szAppCmdLine,     //[in] : cmd line args provided to app
                          const char* szAppCurrentDir,  //[in] : current working dir. for app.
                          char** ppszAppOutput,         //[out] : addr. of a ptr to be filled with a zero-terminated string = console output.  NULL on error.
                          int *piAppExitCode            //[out] : addr. of a int to be filled with app exit code.  INT_MIN on error.
                          );
