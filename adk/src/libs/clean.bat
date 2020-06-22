SET ADK_VER=%~dp0
SET ADK_VER=%ADK_VER%..\..\crescendo_adk
SET LIB_VER=crescendo_libs
%ADK_VER%\tools\bin\make.exe clean -R BLUELAB=%ADK_VER%\tools LIBRARY_VERSION=%LIB_VER% SUPPORTED_EXECUTION_MODES=crescendo