@echo off
set SKYRIM=G:\Steam\steamapps\common\Skyrim Special Edition\Data

copy /Y "%~dp0build\Wanderer.dll" "%SKYRIM%\SKSE\Plugins\Wanderer.dll"
copy /Y "%~dp0dist\Wanderer.esp" "%SKYRIM%\Wanderer.esp"
copy /Y "%~dp0dist\Scripts\WandererMCM.pex" "%SKYRIM%\Scripts\WandererMCM.pex"

if not exist "%SKYRIM%\MCM\Config\Wanderer" mkdir "%SKYRIM%\MCM\Config\Wanderer"
copy /Y "%~dp0dist\MCM\Config\Wanderer\config.json" "%SKYRIM%\MCM\Config\Wanderer\config.json"
copy /Y "%~dp0dist\MCM\Config\Wanderer\settings.ini" "%SKYRIM%\MCM\Config\Wanderer\settings.ini"
